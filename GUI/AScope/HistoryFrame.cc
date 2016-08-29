#include "HistoryFrame.h"

using namespace SideCar::Messages;
using namespace SideCar::GUI::AScope;

void
HistoryFrame::clear()
{
    messages_.clear();
    lastValid_ = -1;
}

void
HistoryFrame::expand()
{
    messages_.append(PRIMessage::Ref());
}

void
HistoryFrame::shrink()
{
    if (! messages_.empty()) {
	messages_.pop_back();
	if (lastValid_ == messages_.size())
	    updateLastValid();
    }
}

void
HistoryFrame::append(const PRIMessage::Ref& msg)
{
    if (msg) {
	lastValid_ = messages_.size();
	setTimeStamp(msg);
    }
    messages_.append(msg);
}

void
HistoryFrame::update(int index, const PRIMessage::Ref& msg)
{
    messages_[index] = msg;
    if (msg) {
	lastValid_ = index;
	setTimeStamp(msg);
    }
}

void
HistoryFrame::clearMessage(int index)
{
    messages_[index].reset();
    if (index == lastValid_)
	updateLastValid();
}

void
HistoryFrame::updateLastValid()
{
    for (int index = 0; index < messages_.size(); ++index) {
	if (messages_[index]) {
	    lastValid_ = index;
	    return;
	}
    }
}

void
HistoryFrame::setTimeStamp(const PRIMessage::Ref& msg)
{
    if (msg->hasIRIGTime())
	timeStamp_ = msg->getIRIGTime();
    else
	timeStamp_ = msg->getCreatedTimeStamp().asDouble();
}
