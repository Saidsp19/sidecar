#ifndef SIDECAR_ALGORITHMS_SHUTDOWNMONITOR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SHUTDOWNMONITOR_H

#include "IO/Module.h"
#include "IO/Task.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Algorithms {

/** Simple task that watches for MB_STOP messages in its message queue, and calls ACE_Reactor::end_event_loop()
    when it sees one.
*/
class ShutdownMonitor : public IO::Task {
public:
    ShutdownMonitor();

    /** Obtain log device to use

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method that creates a new ShutdownMonitor object.

        \return reference to new ShutdownMonitor object
    */
    static Ref Make();

    int close(u_long flags);

protected:
    /** Override of ACE_Task method. Takes a block of raw data read in from a file, and pass it to the algorithm
        under our control.

        \param data raw data to send

        \param timeout amount of time to spend trying to do the send

        \return 0 if successful, -1 otherwise
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    /** Override of IO::Task method. Start a timer to halt ACE event processing loop.

        \return true if successful
    */
    bool doShutdownRequest();

    /** Override of ACE_Event_Handler method. Called when the timer started in doShutdownRequest() ends. Calls
        ACE_Reactor::end_event_loop().

        \param now ignored

        \param act ignored

        \return 0
    */
    int handle_timeout(const ACE_Time_Value& now, const void* act = 0);

private:
    int staleCount_;
};

using ShutdownMonitorModule = IO::TModule<ShutdownMonitor>;

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
