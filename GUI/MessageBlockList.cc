#include "ace/Message_Block.h"

#include "Logger/Log.h"
#include "MessageBlockList.h"

using namespace SideCar;
using namespace SideCar::GUI;

int const MessageBlockList::kMetaTypeId =
    qRegisterMetaType<MessageBlockList::Ref>("MessageBlockList::Ref");

Logger::Log&
MessageBlockList::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.GUI.MessageBlockList");
    return log_;
}

MessageBlockList::MessageBlockList()
    : data_()
{
    static Logger::ProcLog log("MessageBlockList", Log());
    LOGINFO << this << std::endl;
}

MessageBlockList::~MessageBlockList()
{
    static Logger::ProcLog log("~MessageBlockList", Log());
    LOGINFO << this << ' ' << size() << std::endl;
    for (size_t index = 0; index < data_.size(); ++index)
	data_[index]->release();
    data_.clear();
}

ACE_Message_Block*
MessageBlockList::getLastEntry() const
{
    return data_.back()->duplicate();
}

ACE_Message_Block*
MessageBlockList::getEntry(size_t index) const
{
    return data_[index]->duplicate();
}
