#include "QtGui/QFont"

#include "GUI/LogUtils.h"

#include "ControllerItem.h"
#include "RunnerItem.h"
#include "StreamItem.h"

using namespace SideCar::Algorithms;
using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

Logger::Log&
StreamItem::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.StreamItem");
    return log_;
}

StreamItem::StreamItem(const IO::StreamStatus& status, RunnerItem* parent)
    : Super(status, parent), streamIndex_(parent->getNumChildren())
{
    // Create our children
    //
    int taskCount = status.getTaskCount();
    for (int index = 0; index < taskCount; ++index) {

	// Fetch the status for this task item
	//
	const XmlRpc::XmlRpcValue& taskStatus =
	    status[IO::StreamStatus::kTaskStatus][index];

	std::string className = taskStatus[IO::StreamStatus::kClassName];

	TaskItem* child;
	if (className == ControllerStatus::GetClassName()) {
	    child = new ControllerItem(taskStatus, this);
	}
	else {
	    child = new TaskItem(taskStatus, this);
	}

	appendChild(child);
    }
}

void
StreamItem::updateChildren()
{
    const IO::StreamStatus& status(getStatus());
    for (int index = 0; index < getNumChildren(); ++index) {
	TaskItem* child = getChild(index);
	child->update(status.getTaskStatus(index));
    }
}

RunnerItem*
StreamItem::getParent() const
{
    return static_cast<RunnerItem*>(Super::getParent());
}

bool
StreamItem::getParameters(int taskIndex, XmlRpc::XmlRpcValue& definition)
    const
{
    return getParent()->getParameters(streamIndex_, taskIndex, definition);
}

bool
StreamItem::setParameters(int taskIndex, const XmlRpc::XmlRpcValue& updates)
    const
{
    return getParent()->setParameters(streamIndex_, taskIndex, updates);
}

TaskItem*
StreamItem::getChild(int index) const
{
    return static_cast<TaskItem*>(Super::getChild(index));
}

void
StreamItem::formatChangedParameters(const XmlRpc::XmlRpcValue& definitions,
                                    QStringList& changes) const
{
    Logger::ProcLog log("formatChangedParameters", Log());
    LOGINFO << std::endl;
    for (int index = 0; index < getNumChildren(); ++index) {
	const XmlRpc::XmlRpcValue& taskChanges(definitions[index]);
	LOGDEBUG << index << " taskChanges: " << taskChanges.toXml()
		 << std::endl;
	getChild(index)->formatChangedParameters(taskChanges, changes);
    }
}
