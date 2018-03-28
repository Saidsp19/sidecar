#include "boost/bind.hpp"

#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "RGBConverter.h"
#include "RGBConverter_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
RGBConverter::RGBConverter(Controller& controller, Logger::Log& log) :
    Super(controller, log, kDefaultEnabled, kDefaultCpiSpan),
    min_(Parameter::PositiveIntValue::Make("min", "Minimum value of range to normalize on<br>", kDefaultMin)),
    max_(Parameter::PositiveIntValue::Make("max", "Maximum value of range to normalize on<br>", kDefaultMax))
{
    cpiSpan_->connectChangedSignalTo(boost::bind(&RGBConverter::cpiSpanChanged, this, _1));
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the RGBConverter
// class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as
// shown below.
//
bool
RGBConverter::startup()
{
    return registerParameter(min_) && registerParameter(max_) && Super::startup();
}

bool
RGBConverter::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

// This routine is responsible for taking a set of PRIs and computing an encoded RGB value based on the Doppler-range
// values composing this CPI.
//
bool
RGBConverter::processCPI()
{
    static Logger::ProcLog log("processCPI", getLog());
    LOGINFO << std::endl;
    LOGDEBUG << "Process CPI with " << buffer_.size() << " msgs" << std::endl;
    size_t cpiSpan = cpiSpan_->getValue();

    if (buffer_.size() != cpiSpan) {
        LOGWARNING << "Kicking an incomplete CPI!  Expected " << cpiSpan << ", but received " << buffer_.size()
                   << std::endl;
        return true;
    }

    std::deque<Messages::Video::Ref> msg_buffer_;
    for (size_t i = 0; i < buffer_.size(); i++) {
        msg_buffer_.push_back(boost::dynamic_pointer_cast<Messages::Video>(buffer_[i]));
    }

    size_t msg_size = msg_buffer_.front()->size();

    // Build the output message
    //
    Messages::Video::Ref out(Messages::Video::Make(getName(), msg_buffer_[0]));
    Messages::Video::Container& outputData(out->getData());

    // Compute the RGB values for each range across the CPI
    //
    double r, g, b;

    // Get the normalization range parameters
    //
    int min = min_->getValue();
    int max = max_->getValue();
    int span = max - min;

    // Compute where to center the windows for the RGB calculations
    //
    int K = int(::ceil(cpiSpan / 3));
    int R = int(::floor(K / 2));
    int len = 2 * R + 1;

    uint16_t encoded;
    uint16_t r_short;
    uint16_t g_short;
    uint16_t b_short;

    // Pivot points for RGB values are 0, 1/3, 2/3 of cpiSpan
    //
    int pR = K;
    int pG = 0;
    int pB = int(::ceil(2.0 * cpiSpan / 3.0));

    bool rc = true;

    // Compute the summations
    //
    for (size_t i = 0; i < msg_size; i++) {
        r = g = b = 0.0;

        // Handle special case where pivot point is zero and thus wraps around the buffer
        //
        g = msg_buffer_[pG][i];
        for (int j = 0; j < R; j++) {
            g += msg_buffer_[cpiSpan - 1 - j][i];
            g += msg_buffer_[1 + j][i];
        }

        // Compute summations for other R and B windows
        //
        for (int j = 0; j < len; j++) {
            r += msg_buffer_[pR - R + j][i];
            b += msg_buffer_[pB - R + j][i];
        }

        // Compute the average Doppler values
        //
        r /= len;
        g /= len;
        b /= len;

        // Convert to dB
        //
        r = 20.0 * ::log10(r);
        g = 20.0 * ::log10(g);
        b = 20.0 * ::log10(b);

        // Normalize the values, capping vals that fall below/above the min/max user-specified values
        //
        r -= min;
        g -= min;
        b -= min;

        r /= span;
        g /= span;
        b /= span;

        r = (r < 0.0) ? 0.0 : r;
        g = (g < 0.0) ? 0.0 : g;
        b = (b < 0.0) ? 0.0 : b;

        r = (r > 1.0) ? 1.0 : r;
        g = (g > 1.0) ? 1.0 : g;
        b = (b > 1.0) ? 1.0 : b;

        // Encode the normalized values into RGB (655) color scheme where R gets the 6 most significant bits of a a
        // 16-bit number, then G gets the next 5 most significant bits, and then B gets the last 5 bits
        //
        r_short = uint16_t(::round(r * 63));
        g_short = uint16_t(::round(g * 31));
        b_short = uint16_t(::round(b * 31));
        encoded = uint16_t(::rint((r_short << 10) | (g_short << 5) | b_short));

        // Push data into the output buffer
        //
        outputData.push_back(Messages::Video::DatumType(encoded));
    }

    msg_buffer_.clear();

    // Send the output message out
    //
    rc = send(out);

    return rc;
}

bool
RGBConverter::cpiSpanChanged(const Parameter::PositiveIntValue& parameter)
{
    return true;
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[CPIAlgorithm::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the RGBConverter class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
RGBConverterMake(Controller& controller, Logger::Log& log)
{
    return new RGBConverter(controller, log);
}
