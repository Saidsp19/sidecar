#ifndef SIDECAR_ALGORITHMS_RECORDER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_RECORDER_H

#include "IO/Task.h"
#include "IO/Writers.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Algorithms {

/** An ACE service / task that takes data from a processing queue and writes them to a file. Provides a
    Controller object with the ability to record the output of its managed Algorithm object. The Recorder
    performs its data writing in a separate thread, one which is started within the start() method. The writing
    thread takes messages from an internal ACE message queue, and hands the message to a ScatterWriter object
    which performs bulk writes for optimal perfomance. Calling the stop() method will cause the recording thread
    to quit reading from the message queue; the thread then closes the ScatterWriter before exiting and
    terminating itself as a thread.

    A Recorder object will fail to start (returning false from the start()
    method) if a file with the same name as the one given to start() already
    exists on the file system. This is usually due to a misconfiguration of the
    SideCar system, where there are multiple instances of the same processing
    stream. Both streams receive a command to start running, and the Recorder
    to first create its recording file will effectively block the other.
*/
class Recorder : public ACE_Task<ACE_MT_SYNCH> {
    using Super = ACE_Task<ACE_MT_SYNCH>;

public:
    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.
     */
    Recorder(IO::Task& owner);

    bool isActive();

    /** Start the recording process. Opens a connection to a file at the given path, and starts a new thread to
        handle the recording to file.

        \param path the path of the file to record into

        \return true if successful, false otherwise
    */
    bool start(const std::string& path);

    /** Stop the recording process. Deactivates the message queue to signal the thread running the svc() method
        that it is time to exit. Waits for the thread to exit.

        \return true if successful, false otherwise
    */
    bool stop();

    /** Override of ACE_Task method that puts data into the message queue if recording is enabled. Places the
        message to record into the message queue used by the writing thread.

        \param data value to add to the message queue

        \param timeout amount of time to wait to add before failing

        \return -1 if error
    */
    int put(ACE_Message_Block* data, ACE_Time_Value* timeout = 0) { return putq(data, timeout); }

private:
    /** Override of ACE_Task method. Processing any entries in the message queue by writing them to file. NOTE:
        this routine is run in its own thread.

        \return 0 if successful, -1 otherwise
    */
    int svc();

    IO::Task& owner_;       ///< The task that will receive our errors
    IO::FileWriter writer_; ///< Object that does the actual writing
};

} // namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
