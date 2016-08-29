#include "Logger/Log.h"

#include "IOTask.h"
#include "MessageManager.h"

using namespace SideCar::IO;
using namespace SideCar::Messages;

Logger::Log&
IOTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.IOTask");
    return log_;
}

void
IOTask::acquireExternalMessage(ACE_Message_Block* data)
{
    static Logger::ProcLog log("acquireExternalMessage", Log());
    LOGINFO << "taskIndex: " << getTaskIndex() << " data: " << data << " length: " << data->length() << std::endl;

    // We want to setup the message so that it gets delivered to those tasks connected on our first output
    // channel. We also want to update our processing stats.
    //
    MessageManager mgr(data, getMetaTypeInfo());

    // Update input statistics
    //
    Header::Ref msg(mgr.getNative());
    updateInputStats(0, msg->getSize(), msg->getMessageSequenceNumber());
    sendManaged(mgr, 0);
}

void
IOTask::establishedConnection()
{
    enterLastProcessingState();
    updateUsingDataValue();
}
