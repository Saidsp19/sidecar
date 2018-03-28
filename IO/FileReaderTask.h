#ifndef SIDECAR_IO_FILEREADERTASK_H // -*- C++ -*-
#define SIDECAR_IO_FILEREADERTASK_H

#include "IO/IOTask.h"
#include "IO/Module.h"
#include "IO/Readers.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** An ACE service / task that takes data from a file and generates messages objects which are then placed on a
    processing queue.
*/
class FileReaderTask : public IOTask {
    using Super = IOTask;

public:
    using Ref = boost::shared_ptr<FileReaderTask>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new FileReaderTask objects

        \return reference to new FileReaderTask object
    */
    static Ref Make();

    /** Open a file for reading

        \param key message type ID assigned to the reader

        \param path location of the file to open

        \param signalEndOfFile if true, stop the ACE Reactor event loop when we encounter EOF.

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, const std::string& path, bool signalEndOfFile = true,
                     long threadFlags = kDefaultThreadFlags, long threadPriority = ACE_DEFAULT_THREAD_PRIORITY);

    bool start();

    /** Override of ACE_Task method. The service is begin shutdown. Close the file connection.

        \param flags if 1, module is shutting down.

        \return 0 if successful, -1 otherwise.
    */
    int close(u_long flags = 0);

protected:
    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
        methods.
    */
    FileReaderTask();

    /** Override of ACE_Event_Handler method. Returns the connection descriptor associated with the reader
        object.

        \return connection descriptor
    */
    ACE_HANDLE get_handle() const { return reader_.getDevice().get_handle(); }

    /** Override of ACE_Task method. Continuously pulls messages from the input file and puts them on the next
        task's processing queue. Runs in a separate thread.

        \return 0 always
    */
    int svc();

    bool enterRunState();

private:
    /** Implementation of Task::deliverDataMessage() method.

        \param data raw data to send

        \param timeout amount of time to spend trying to do the send

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    FileReader reader_;    ///< Object that does actual reading of data
    bool signalEndOfFile_; ///< Stop the ACE event loop on EOF
    long threadFlags_;
    long threadPriority_;
    volatile bool active_; ///< True if thread is running
};

using FileReaderTaskModule = TModule<FileReaderTask>;

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
