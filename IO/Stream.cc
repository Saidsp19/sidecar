#include "IO/ProcessingState.h"
#include "IO/StatusEmitterBase.h"
#include "IO/StreamStatus.h"
#include "IO/TaskStatus.h"
#include "Logger/Log.h"

#include "Module.h"
#include "Stream.h"

using namespace SideCar::IO;

Logger::Log&
Stream::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.Stream");
    return log_;
}

Task::Ref
Stream::getTask(int index) const
{
    ACE_Stream_Iterator<ACE_MT_SYNCH> iter(*this);

    // Skip over the stream head.
    //
    iter.advance();
    while (! iter.done() && index > 0) {
	iter.advance();
	--index;
    }

    // Fetch the module that we are now pointing to. However, verify that we are not pointing to the tail
    // module, and if not then return the module's Task object.
    //
    const ACE_Module<ACE_MT_SYNCH>* module;
    iter.next(module);
    iter.advance();
    if (! iter.done())
	return static_cast<const Module*>(module)->getTask();

    return Task::Ref();
}	

void
Stream::fillStatus(StatusBase& status)
{
    static Logger::ProcLog log("fillStatus", Log());
    LOGINFO << std::endl;

    XmlRpc::XmlRpcValue::ValueArray* taskStatusArray =
	new XmlRpc::XmlRpcValue::ValueArray;

    ACE_Stream_Iterator<ACE_MT_SYNCH> iter(*this);

    // Skip over the stream head.
    //
    iter.advance();
    while (! iter.done()) {
	const ACE_Module<ACE_MT_SYNCH>* module;
	iter.next(module);
	iter.advance();

	// If we were just pointing at the tail, then quit.
	//
	if (iter.done()) break;

	Task::Ref task(static_cast<const Module*>(module)->getTask());
	StatusBase status(task->getStatusSize(), task->getStatusClassName(),
                          task->getTaskName());
	task->fillStatus(status);
	LOGDEBUG << "task status: " << status << std::endl;
	taskStatusArray->push_back(status.getXMLData());
    }

    status.setSlot(StreamStatus::kTaskStatus, taskStatusArray);
}

void
Stream::getChangedParameters(XmlRpc::XmlRpcValue& value) const
{
    Logger::ProcLog log("getChangedParameters", Log());
    LOGINFO << getName() << std::endl;

    ACE_Stream_Iterator<ACE_MT_SYNCH> iter(*this);

    // Make sure we write changes into an XML-RCP array.
    //
    value.setSize(0);

    // Skip over the stream head.
    //
    iter.advance();
    while (! iter.done()) {
	const ACE_Module<ACE_MT_SYNCH>* module;
	iter.next(module);
	iter.advance();

	// If we were just pointing at the tail, then quit.
	//
	if (iter.done()) break;

	Task::Ref task(static_cast<const Module*>(module)->getTask());
	XmlRpc::XmlRpcValue changes;
	task->getChangedParameters(changes);
	LOGDEBUG << "changes: " << changes.toXml() << std::endl;
	value.push_back(changes);
    }
}

void
Stream::emitStatus() const
{
    Logger::ProcLog log("emitStatus", Log());
    LOGINFO << emitter_ << std::endl;
    if (emitter_) emitter_->emitStatus();
}
