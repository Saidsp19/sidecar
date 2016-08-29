#ifndef SIDECAR_ALGORITHMS_BUGCOLLECTOR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_BUGCOLLECTOR_H

#include "boost/scoped_ptr.hpp"

#include "Algorithms/Algorithm.h"
#include "Messages/BugPlot.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

namespace BugCollectorUtils { class BugPlotSubscriber; }

/** The BugCollector algorithm takes automatically subscribes to all available BugPlot publishers, and sends
    them out on its only stream channel, presumably to be published by a connected DataPublisher object.

    Since we don't want to subscribe to ourselves, we only subscribe to those
    BugPlot publishers that have a particular prefix in their name. This is a
    runtime paramter called 'prefix'.
*/
class BugCollector : public Algorithm
{
    using Super = Algorithm;
public:

    enum InfoSlot {
	kActive = ControllerStatus::kNumSlots,
	kPrefix,
	kNumSlots
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    BugCollector(Controller& controller, Logger::Log& log);

    /** Override of Algorithm::startup(). Registers run-time parameters and incoming message processors with the
        controller.

        \return true if successful.
    */
    bool startup();

    /** Process incoming messages. For the BugCollector, this method simply calls the Algorithm::send() method
        for each message.

        \param msg incoming message to process
    */
    void process(const Messages::BugPlot::Ref& msg);

private:

    size_t getNumInfoSlots() const { return kNumSlots; }

    /** Override of Algorithm::setInfoSlots(). Stores XML representation of cancellation statistics into the
        given XML-RPC container.

        \param status XML-RPC container to hold the stats
    */
    void setInfoSlots(IO::StatusBase& status);

    /** Notification handler called when the prefix parameter changes.

        \param parameter reference to parameter that changed
    */
    void prefixChanged(const Parameter::StringValue& parameter);

    boost::scoped_ptr<BugCollectorUtils::BugPlotSubscriber> subscriber_;
    Parameter::StringValue::Ref prefix_;
};

}} // namespace SideCar::Algorithms

/** \file
 */

#endif
