#include "Logger/Log.h"

#include "Channel.h"
#include "Task.h"

using namespace SideCar;
using namespace SideCar::IO;

Logger::Log&
Channel::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.Channel");
    return log_;
}

void
Channel::updateSenderUsingData(bool recipientValue) const
{
    Logger::ProcLog log("updateSenderUsingData", Log());
    LOGINFO << name_ << " recipientValue: " << recipientValue << std::endl;

    // Modify the sender Task so that it will emit data if asked to, or if one or more recipients asks it to.
    //
    Task* sender = sender_.get();
    if (sender != nullptr) sender->setUsingData(recipientValue);
}

Logger::Log&
ChannelVector::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.ChannelVector");
    return log_;
}

void
ChannelVector::add(const Channel& channel)
{
    static Logger::ProcLog log("add", Log());
    LOGINFO << "name: " << channel.getName() << std::endl;

    if (findName(channel.getName()) < container_.size()) {
	Utils::Exception ex(" duplicate channel name - ");
	ex << channel.getName();
	log.thrower(ex);
    }

    container_.push_back(channel);
}

size_t
ChannelVector::findName(const std::string& name) const
{
    static Logger::ProcLog log("findName", Log());
    LOGINFO << "name: " << name << std::endl;

    for (size_t index = 0; index < container_.size(); ++index) {
	if (container_[index].getName() == name) return index;
    }

    return container_.size();
}

void
ChannelVector::updateSendersUsingData(bool state)
{
    Logger::ProcLog log("updateSendersUsingData", Log());
    LOGINFO << "state: " << state << std::endl;

    // Visit all of the Channel objects and update their sender Task objects with the given data-pulling state.
    //
    for (size_t index = 0; index < container_.size(); ++index) {
	container_[index].updateSenderUsingData(state);
    }
}

bool
ChannelVector::areAnyRecipientsUsingData() const
{
    // Locate the first Channel with a recipient that is pulling data.
    //
    for (size_t index = 0; index < container_.size(); ++index) {
	if (container_[index].areAnyRecipientsUsingData()) {
	    return true;
        }
    }

    return false;
}
