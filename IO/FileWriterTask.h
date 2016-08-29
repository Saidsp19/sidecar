#ifndef SIDECAR_IO_FILEWRITERTASK_H // -*- C++ -*-
#define SIDECAR_IO_FILEWRITERTASK_H

#include "IO/IOTask.h"
#include "IO/Module.h"
#include "IO/Writers.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

/** An ACE service / task that takes data from a processing queue writes them out to a file.
 */
class FileWriterTask : public IOTask
{
    using Super = IOTask;
public:
    using Ref = boost::shared_ptr<FileWriterTask>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new FileWriterTask objects

        \return reference to new FileWriterTask object
    */
    static Ref Make();

    /** Open a file for writing, and starts a separate thread to handle the writing unless an error was found.

        \param key message type key for data going to file

        \param path location of the file to open

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, const std::string& path, bool acquireBasisTimeStamps = true,
                     long threadFlags = kDefaultThreadFlags, long threadPriority = ACE_DEFAULT_THREAD_PRIORITY);

    /** Override of ACE_Task method. The service is being shutdown. Close the file connection.
        
	\param flags if 1, signal service thread to shutdown.

        \return 0 if successful, -1 otherwise.
    */
    int close(u_long flags = 0);

    /** Override of Task::setUsingData() method. Forces valve to be true.
        
        \param state true if task is using data from upstream tasks
    */
    void setUsingData(bool state) { Super::setUsingData(true); }

protected:

    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
        methods.
    */
    FileWriterTask()
	: Super(), writer_(), acquireBasisTimeStamps_(true) {}

    /** Override of ACE_Task method. Processing any entries in the message queue by writing them to file. NOTE:
        this routine is run in its own thread.
        
        \return 0 if successful, -1 otherwise
    */
    int svc();

private:

    /** Implementation of Task::deliverDataMessage() method.
        
        \param data message to deliver

        \param timeout amount of time to spend trying to deliver message

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    FileWriter writer_;	       ///< Object that does the actual writing of data
    bool acquireBasisTimeStamps_;
};

using FileWriterTaskModule = TModule<FileWriterTask>;

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
