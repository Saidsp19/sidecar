#include "IO/MessageManager.h"
#include "IO/Task.h"

#include "Messages/MetaTypeInfo.h"

#include "RTCLMessageReader.h"

#include "LogUtils.h"

using namespace SideCar;
using namespace SideCar::GUI;

Logger::Log&
RTCLMessageReader::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.GUI.RTCLMessageReader");
    return log_;
}

RTCLMessageReader*
RTCLMessageReader::Make(const std::string& topic,
                        const Messages::MetaTypeInfo* type)
{
    static Logger::ProcLog log("Make", Log());
    LOGINFO << std::endl;

    IO::Task::Ref reader = type->makeDDSSubscriber(
	Messages::MetaTypeInfo::kRTCL, topic, IO::Task::kDefaultThreadFlags,
	ACE_DEFAULT_THREAD_PRIORITY);
    if (! reader) {
	LOGERROR << "failed to open reader" << std::endl;
	return 0;
    }

    return new RTCLMessageReader(type, reader);
}

RTCLMessageReader::RTCLMessageReader(const Messages::MetaTypeInfo* type,
                                     const IO::Task::Ref& reader)
    : Super(type), reader_(reader)
{
    static Logger::ProcLog log("RTCLMessageReader", Log());
    LOGINFO << std::endl;

    // Make ourselves the next task for the RTCLMessageReader task so that his put_next() will invoke our put()
    // method.
    //
    reader_->next(this);
}

int
RTCLMessageReader::put(ACE_Message_Block* data, ACE_Time_Value*)
{
    // MessageManager takes ownership of given ACE_Message_Block so we don't have to do a release() on it.
    //
    IO::MessageManager mgr(data);
    emit received(mgr.getNative());
    return 0;
}
