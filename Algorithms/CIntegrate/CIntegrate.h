#ifndef SIDECAR_ALGORITHMS_CINTEGRATE_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CINTEGRATE_H

#include <algorithm>
#include <deque>
#include <vector>

#include "boost/scoped_ptr.hpp"

#include <vsip/support.hpp>
#include <vsip/matrix.hpp>
#include <vsip/vector.hpp>
#include <vsip/signal.hpp>
#include <vsip/math.hpp>

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm CIntegrate. Please describe what the algorithm does, in layman's terms and,
    if possible, mathematical terms.
*/
class CIntegrate : public Algorithm
{
    using Super = Algorithm;
    using ComplexType = std::complex<float>;
    using VsipComplexVector = vsip::Vector<ComplexType>;
    using VsipComplexMatrix = vsip::Matrix<ComplexType>;
    using MessageQueue = std::deque<Messages::Video::Ref>;

    /** Specification for a forward FFT processor. This operates on complex values (I,Q), but processes
     *columns* of values at a time, not rows, in order to generate frequency information across PRIs at the
     same range. Also, by default, the processor attempts to be as fast as possible, so initial results may
     be slow coming out.
    */
    using ForwardFFTM = vsip::Fftm<ComplexType, ComplexType, vsip::col, vsip::fft_fwd, vsip::by_reference>;

public:

    /** Enumeration of info slots provided by this algorithm
     */
    enum InfoSlots {
        kEnabled = ControllerStatus::kNumSlots,
	kMin,
	kMax,
	kNumPRIs,
	kColorEncoding,
	kWindowShift,
	kDopplerBin,
        kNumSlots
    };

    /** Enumeration of available encodingings from magnitudes into RGB values
     */
    enum ColorEncoding {
        kColorEncodingMinValue,
        kHSV = kColorEncodingMinValue,
        kRGB,
        kColorEncodingMaxValue = kRGB,
        kNumColorEncodings
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    CIntegrate(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::startup interface. Clears message buffer.

        \return true if successful, false otherwise
    */
    bool reset();

    /** Manually set the number of PRIs used for processing a CPI

        \param v new value
    */
    void setNumPRIs(int v) { numPRIs_->setValue(v); }

    /** Manually set the number of PRIs to shift over after processing a CPI.

        \param v new value
    */
    void setSlidingWindowShift(int v) { moveSlidingWin_->setValue(v); }

    /** Manually set the type of color encoding used.

        \param v new value
    */
    void setEncoding(ColorEncoding v) { colorEncoding_->setValue(v); }

private:

    /** Convert FFT magnitude values into RGB values. Uses contents of the fftOut_ matrix attribute.

	\param out storage for calculated values
    */
    void processRGB(Messages::Video::Container& out);

    /** Convert FFT magnitude values into HSV values. Uses contents of the fftOut_ matrix attribute.

	\param out storage for calculated values
    */
    void processHSV(Messages::Video::Container& out);

    /** Compute the maximum amplitude values for each FFT, storing the I,Q values of the largest sample.

        \param out storage for I,Q values
    */
    void processMaxAmp(Messages::Video::Container& out);

    /** Copy a slice across the FFT results into the given buffer. The dopplerBin_ parameter controls which
        slice to copy.

        \param out storage for I,Q values
    */
    void processOneDopplerBin(Messages::Video::Container& out);

    /** Recalculate the scaleMin_ and scaleFactor_ values due to changes in min_ and/or max_ parameter values.
     */
    void scalingChanged();

    /** Reallocate the matrices used for FFT processing, and recalculate the hue vector attribute H_.
     */
    void dimensionsChanged();

    /** Obtain amplitude value between min_ and max_ parameter values.

        \param amp the value to convert

        \return scaled result
    */
    float scaleAmp(float amp) const {
	return std::max(std::min((amp - scaleMin_) * scaleFactor_, 1.0f),
                        0.0f);
    }

    /** Notification callback invoked when the numPRIs value changes. Causes recalculation and allocation of
	internal attributes that depend on it. parameter the parameter that changed
    */
    void numPRIsChanged(const Parameter::PositiveIntValue& parameter);

    /** Notification callback invoked when the min_ value changes. Causes recalculation of the scaleMin_ and
	scaleFactor_ attributes. parameter the parameter that changed
    */
    void minValueChanged(const Parameter::NonNegativeIntValue& parameter);

    /** Notification callback invoked when the max_ value changes. Causes recalculation of the scaleFactor_
	attribute. parameter the parameter that changed
    */
    void maxValueChanged(const Parameter::NonNegativeIntValue& parameter);

    /** Override of Algorithm method. Obtain the total number of informational slots provided by this algorithm
	(including those provided by ancestor classes)

	\return total slot count
    */
    size_t getNumInfoSlots() const { return kNumSlots; }

    /** Override of Algorithm method. Fills in the informational slots provided by this algorithm.

	\param status storage for status updates
    */
    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    /** Encode normalized floating point values for red, green, and blue into a 16-bit integer value, where red
	occupies the top 6 bits, green the next 5 bits followed by blue in the least-significant 5 bits.

	\param r the red component value between 0.0 and 1.0

	\param g the green component value between 0.0 and 1.0

	\param b the blue component value between 0.0 and 1.0

	\return encoded value
    */
    static uint16_t EncodeRGB(float r, float g, float b)
	{
	    uint16_t ri = ::round(std::min(r, 1.0f) * 63);
	    uint16_t gi = ::round(std::min(g, 1.0f) * 31);
	    uint16_t bi = ::round(std::min(b, 1.0f) * 31);
	    return (ri << 10) | (gi << 5) | bi;
	}

    /** Controls whether CPI processing is enabled
     */
    Parameter::BoolValue::Ref enabled_;

    /** Minimum value for scaling amplitude values.
     */
    Parameter::NonNegativeIntValue::Ref min_;

    /** Maximum value for scaling amplitude values.
     */
    Parameter::NonNegativeIntValue::Ref max_;

    /** Number of PRIs used in a CPI processing window.
     */
    Parameter::PositiveIntValue::Ref numPRIs_;

    /** Traits definition for the ColorEncoding enumeration.
     */
    struct ColorEncodingEnumTraits :
	public Parameter::Defs::EnumTypeTraitsBase {
	using ValueType = ColorEncoding;
	static ValueType GetMinValue() { return kColorEncodingMinValue; } 
	static ValueType GetMaxValue() { return kColorEncodingMaxValue; }
	static const char* const* GetEnumNames();
    };

    /** Type definition for the ColorEncoding parameter.
     */
    using ColorEncodingParameter = Parameter::TValue<Parameter::Defs::Enum<ColorEncodingEnumTraits>>;

    /** Controls which routine to use for RGB encoding.
     */
    ColorEncodingParameter::Ref colorEncoding_;

    /** Number of messages to shift the CPI processing window.
     */
    Parameter::PositiveIntValue::Ref moveSlidingWin_;

    /** The row of the FFT matrix to emit on the dopplerBinOutput channel.
     */
    Parameter::NonNegativeIntValue::Ref dopplerBin_;

    /** Input matrix used for FFT calculations
     */
    boost::scoped_ptr<VsipComplexMatrix> PRIs_;

    /** Output matrix used for FFT calculations
     */
    boost::scoped_ptr<VsipComplexMatrix> fftOut_;

    /** FFT calculator. See type specification for ForwardFFTM for additional commentary.
     */
    boost::scoped_ptr<ForwardFFTM> fftm_;

    size_t N_;			///< Value of numPRIs_ parameter
    size_t maxSpan_;		///< Value of largest PRI message seen
    MessageQueue buffer_;    	///< Buffer of incoming messages
    std::vector<double> H_;	///< Hue mapping for HSV processing

    size_t resultsIndex_;	///< Channel index for results output
    size_t rgbIndex_;		///< Channel index for RGB output
    size_t maxAmpIndex_;	///< Channel index for maxAmp output
    size_t dopplerBinOutIndex_;	///< Channel index for doppler bin output
    size_t kInvalidChannel_;	///< Invalid channel index
    float scaleMin_;		///< Value of min_ parameter
    float scaleFactor_;		///< Scaling factor from min_ and max_ params
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
