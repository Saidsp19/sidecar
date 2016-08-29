#ifndef SIDECAR_ALGORITHMS_SANDBOX_H	// -*- C++ -*-
#define SIDECAR_ALGORITHMS_SANDBOX_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Messages/BinaryVideo.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** A playground for for testing out algorithms. May change at any time.
 */
class SandBox : public Algorithm
{
public:

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    SandBox(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

private:

    bool processOne(const Messages::Video::Ref& msg);
    bool processTwo(const Messages::Video::Ref& msg);
    bool processThree(const Messages::Video::Ref& msg);
    bool processFour(const Messages::Video::Ref& msg);

    void intValueChanged(const Parameter::IntValue& value);
    void boolValueChanged(const Parameter::BoolValue& value);
    void pathValueChanged(const Parameter::ReadPathValue& value);
    void notificationValueChanged(const Parameter::NotificationValue& value);

    /** Definition of the Bogus range. Since double values cannot be template parameters (unlike ints), the
	Parameter::Defs::RangedDouble class requires a traits class that defines GetMinValue and GetMaxValue to
	define the parameter value range.
    */
    struct Range {
	static double GetMinValue() { return -1.0; }
	static double GetMaxValue() { return  1.0; }
    };

    size_t oneChannelIndex_;
    size_t twoChannelIndex_;
    size_t threeChannelIndex_;
    size_t fourChannelIndex_;

    Parameter::IntValue::Ref intValue_;
    Parameter::BoolValue::Ref boolValue_;
    using DRange = Parameter::TValue<Parameter::Defs::RangedDouble<Range> >;
    DRange::Ref doubleRange_;
    Parameter::ReadPathValue::Ref pathValue_;
    Parameter::NotificationValue::Ref notificationValue_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
