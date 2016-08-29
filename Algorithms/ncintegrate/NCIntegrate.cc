#include <cmath>
#include <iterator>

#include "Utils/VsipVector.h"
#include <vsip/math.hpp>

#include "boost/bind.hpp"

#include "Logger/Log.h"

#include "NCIntegrate.h"
#include "NCIntegrate_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Messages;
using namespace SideCar::Algorithms;

NCIntegrate::NCIntegrate(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
      numPulses_(Parameter::PositiveIntValue::Make("numPRIs", "Num PRIs", kDefaultNumPRIs)),
      iqValues_(Parameter::BoolValue::Make("iqValues", "IQ Values", kDefaultIqValues)),
      in_(kDefaultNumPRIs), runningAverage_()
{
    numPulses_->connectChangedSignalTo(boost::bind(&NCIntegrate::numPulsesChanged, this, _1));
    iqValues_->connectChangedSignalTo(boost::bind(&NCIntegrate::iqValuesChanged, this, _1));
}

bool
NCIntegrate::startup()
{
    registerProcessor<NCIntegrate,Video>(&NCIntegrate::process);
    return registerParameter(enabled_) &&
	registerParameter(iqValues_) &&
	registerParameter(numPulses_) &&
	Algorithm::startup();
}

bool
NCIntegrate::reset()
{
    in_.clear();
    runningAverage_.clear();
    return true;
}

/** Text Stream insertion operator for a RunningAverageVector. Prints out the values of the vector.
 */
inline std::ostream&
operator<<(std::ostream& os, const NCIntegrate::RunningAverageVector& rav)
{
    Traits::GenericPrinter<NCIntegrate::RunningAverageVector::value_type,10>(os, rav);
    return os;
}

/** Base class for the averaging functors used to update the running average vector.
 */
struct AverageBase
{
    using DataType = NCIntegrate::RunningAverageVector::value_type;
    int32_t size;
    int32_t halfSize;
    Video::const_iterator in;
    Video::Container& out;
    
    /** Constructor

        \param s number of messages being integrated

        \param i input iterator of the message being added

        \param o outputiterator of the message being built
    */
    AverageBase(int32_t s, Video::const_iterator i, Video::Container& o)
	: size(s), halfSize(s / 2), in(i), out(o) {}

    /** Convenience function for IQ processing. Returns the square of the given value.

	\param x value to work with

	\return squared value
    */
    static DataType squared(DataType x) { return x * x; }

    /** Convenience function for IQ processing. Returns the square root of the given value, rounded to the
	nearest integer.

	\param x value to work with

	\return rounded squared-root value
    */
    static DataType root(DataType x) { return ::round(::sqrt(x)); }
};

/** Functor used to update runningAverage_ by adding new message data and removing the oldest message, and as a
    side-effect add running average values to an output message.
*/
struct Average3 : public AverageBase
{
    Video::const_iterator gone;

    Average3(int32_t s, Video::const_iterator i, Video::const_iterator g, Video::Container& o)
	: AverageBase(s, i, o), gone(g) {}

    DataType operator()(DataType x)
	{
	    DataType v = x + *in++ - *gone++;
	    out.push_back((v + halfSize) / size);
	    return v;
	}
};

/** Functor used to update runningAverage_ by adding new IQ message data and removing the oldest message, and as
    a side-effect add running average values to an output message.
*/
struct Average3IQ : public AverageBase
{
    Video::const_iterator gone;

    Average3IQ(int32_t s, Video::const_iterator i, Video::const_iterator g,
               Video::Container& o)
	: AverageBase(s, i, o), gone(g) {}

    DataType operator()(DataType x)
	{
	    DataType newI = *in++;
	    DataType newQ = *in++;
	    DataType oldI = *gone++;
	    DataType oldQ = *gone++;
	    DataType v = x + (squared(newI) + squared(newQ)) - (squared(oldI) + squared(oldQ));
	    out.push_back(root((v + halfSize) / size));
	    return v;
	}
};

/** Functor used to update runningAverage_ by adding new message data, and as a side-effect add running average
    values to an output message.
*/
struct Average2 : public AverageBase
{
    Average2(int32_t s, Video::const_iterator i, Video::Container& o)
	: AverageBase(s, i, o) {}

    DataType operator()(DataType x)
	{
	    DataType v = x + *in++;
	    out.push_back((v + halfSize) / size);
	    return v;
	}
};

/** Functor used to update runningAverage_ by adding new IQ message data, and as a side-effect add running
    average values to an output message.
*/
struct Average2IQ : public AverageBase
{
    Average2IQ(int32_t s, Video::const_iterator i, Video::Container& o)
	: AverageBase(s, i, o) {}

    DataType operator()(DataType x)
	{
	    DataType newI = *in++;
	    DataType newQ = *in++;
	    DataType v = x + squared(newI) + squared(newQ);
	    out.push_back(root((v + halfSize) / size));
	    return v;
	}
};

bool
NCIntegrate::process(Video::Ref msg)
{
    static Logger::ProcLog log("process", getLog());
    LOGINFO << std::endl;
    LOGDEBUG << *msg.get() << std::endl;

    if (! enabled_->getValue() || numPulses_->getValue() == 1) {
	bool rc = send(msg);
	LOGDEBUG << "rc: " << rc << std::endl;
	return rc;
    }

    Video::Ref gone;
    if (in_.full()) {
	gone = in_.back();
    }

    in_.add(msg);

    if (in_.size() == 1) {

	// This is the first message, just copy over its sample values into our running average buffer.
	//
	LOGDEBUG << "first time" << std::endl;
	if (iqValues_->getValue()) {
	    Video::const_iterator pos = msg->begin();
	    Video::const_iterator end = msg->end();
	    while (pos < end) {
		RunningAverageVector::value_type newI = *pos++;
		RunningAverageVector::value_type newQ = *pos++;
		runningAverage_.push_back(newI * newI + newQ * newQ);
	    }
	}
	else {
	    std::copy(msg->begin(), msg->end(), std::back_inserter<>(runningAverage_));
	}
	LOGDEBUG << runningAverage_ << std::endl;
	return true;
    }

    // Make sure we don't address out-of-bounds values.
    //
    size_t limit = runningAverage_.size();
    if (msg->size() < limit) {
	limit = msg->size();
	if (iqValues_->getValue()) {
	    limit /= 2;
        }
    }
    else if (msg->size() > limit) {

	// Expand our runningAverage_ to the largest message ever seen.
	//
	limit = msg->size();
	if (iqValues_->getValue()) {
	    limit /= 2;
        }
	runningAverage_.resize(limit, 0);
    }

    if (! in_.full()) {

	// Just update runningAverage_ by adding the new message to it.
	//
	if (iqValues_->getValue()) {
	    RunningAverageVector::iterator pos = runningAverage_.begin();
	    RunningAverageVector::iterator end = runningAverage_.begin() + limit;
	    Video::const_iterator vit = msg->begin();
	    while (pos != end) {
		RunningAverageVector::value_type i = *vit++;
		RunningAverageVector::value_type q = *vit++;
		*pos++ += i * i + q * q;
	    }
	}
	else {
	    std::transform(runningAverage_.begin(), runningAverage_.begin() + limit, msg->begin(),
                           runningAverage_.begin(), std::plus<int32_t>());
	}
	LOGDEBUG << runningAverage_ << std::endl;
	return true;
    }

    // Base our outgoing message on the middle message in our retention queue.
    //
    Video::Ref midPoint(*(in_.begin() + in_.size() / 2));
    Video::Ref out(Video::Make(getName(), midPoint));

    // Make sure we only allocate once for the samples we are about to generate.
    //
    out->reserve(runningAverage_.size());

    if (gone) {

	// Update runningAverage by adding to it the newest input message and subtracting the oldest input message, and
	// fill the output message with the calculated running average.
	//
	if (iqValues_->getValue()) {
	    std::transform(runningAverage_.begin(), runningAverage_.end(), runningAverage_.begin(),
                           Average3IQ(in_.size(), msg->begin(), gone->begin(), out->getData()));
	}
	else {
	    std::transform(runningAverage_.begin(), runningAverage_.end(), runningAverage_.begin(),
                           Average3(in_.size(), msg->begin(), gone->begin(), out->getData()));
	}
    }
    else {

	// Update runningAverage by adding to it the newest input message, and fill the output message with the
	// calculated running average.
	//
	if (iqValues_->getValue()) {
	    std::transform(runningAverage_.begin(), runningAverage_.end(), runningAverage_.begin(),
                           Average2IQ(in_.size(), msg->begin(), out->getData()));
	}
	else {
	    std::transform(runningAverage_.begin(), runningAverage_.end(), runningAverage_.begin(),
                           Average2(in_.size(), msg->begin(), out->getData()));
	}
    }

    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

#if 0

bool
NCIntegrate::process(Video::Ref msg)
{
    static Logger::ProcLog log("process", getLog());
    LOGINFO << std::endl;
    LOGDEBUG << *msg.get() << std::endl;

    if (! enabled_->getValue() || numPulses_->getValue() == 1) {
	bool rc = send(msg);
	LOGDEBUG << "rc: " << rc << std::endl;
	return rc;
    }

    Video::Ref gone;
    if (in_.full()) {
	gone = in_.back();
    }

    in_.add(msg);
    if (in_.size() == 1) {

	// This is the first message, just copy over its sample values into our running average buffer.
	//
	LOGDEBUG << "first time" << std::endl;
	std::copy(msg->begin(), msg->end(),
                  std::back_inserter<>(runningAverage_));
	LOGDEBUG << runningAverage_ << std::endl;
	return true;
    }

    VsipVector<Video> vMsg(*msg);
    VsipVector<RunningAverageVector> vAve(runningAverage_);
    vMsg.admit(true);		// Bring in values from msg vector
    vAve.admit(true);		// Bring in values from runningAverage vector
    vAve.v += vMsg.v;
    vMsg.release(false);	// Don't need these values from vsip

    if (! in_.full()) {

	// Not enough PRIs to send out a message, so just add to the running average vector.
	//
	vAve.release(true);	// Bring back values to runningAverage

	LOGDEBUG << runningAverage_ << std::endl;
	return true;
    }

    // Base our outgoing message on the middle message in our retention queue.
    //
    Video::Ref midPoint(*(in_.begin() + in_.size() / 2));
    Video::Ref out(Video::Make(getName(), midPoint));
    out->resize(in_.getMaxMsgSize(), 0);
    VsipVector<Video> vOut(*out);
    vOut.admit(false);	// Don't bring in values from out vector

    if (gone) {

        // We have to remove the effect of the oldest message sample data while calculating the new average.
        //
        VsipVector<Video> vOld(*gone);
        vOld.admit(true);
        vAve.v -= vOld.v;
        vOld.release(false);
    }

    // Add in half buffer size before dividing to handle rounding up.
    //
    int half = in_.size() / 2;
    vOut.v = (vAve.v + half) / in_.size();

    // Advise VSIPL to flush internal buffers to external vector. Only necessary for runningAverage_ and out
    // vectors.
    // 
    vAve.release(true);
    LOGDEBUG << runningAverage_ << ' ' << in_.size() << std::endl;

    vOut.release(true);
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

#endif

void
NCIntegrate::numPulsesChanged(const Parameter::PositiveIntValue& value)
{
    static Logger::ProcLog log("numPulsesChanged", getLog());
    size_t newSize = value.getValue();
    LOGINFO << "new value: " << newSize << std::endl;

    in_.setCapacity(newSize);
    in_.clear();
    runningAverage_.clear();
}

void
NCIntegrate::iqValuesChanged(const Parameter::BoolValue& value)
{
    static Logger::ProcLog log("iqValuesChanged", getLog());
    in_.clear();
    runningAverage_.clear();
}

void
NCIntegrate::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kNumPRIs, int(in_.getCapacity()));
    status.setSlot(kIQValues, iqValues_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    int numPRIs = status[NCIntegrate::kNumPRIs];
    bool iqValues = status[NCIntegrate::kIQValues];
    QString out = QString("Num PRIs: %1").arg(numPRIs);
    if (iqValues) out += " *IQ*";
    return Algorithm::FormatInfoValue(out);
}

extern "C" ACE_Svc_Export Algorithm*
NCIntegrateMake(Controller& controller, Logger::Log& log)
{
    return new NCIntegrate(controller, log);
}
