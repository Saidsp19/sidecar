#include "ace/Message_Block.h"

#include "Logger/Log.h"

#include "RecipientList.h"
#include "Task.h"

using namespace SideCar::IO;

Logger::Log&
RecipientList::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.RecipientList");
    return log_;
}

RecipientList::RecipientList() : entries_()
{
    ;
}

void
RecipientList::add(const TaskRef& task, size_t channelIndex)
{
    static Logger::ProcLog log("add", Log());
    LOGINFO << "task: " << task << " channelIndex: " << channelIndex << std::endl;

    // NOTE: no check for duplicate entries. Assumed to be done elsewhere
    //
    entries_.push_back(Entry(task, channelIndex));
}

std::ostream&
RecipientList::print(std::ostream& os) const
{
    for (size_t index = 0; index < entries_.size(); ++index) {
        os << "[" << entries_[index].task_ << "," << entries_[index].channelIndex_ << "]";
    }

    return os;
}

bool
RecipientList::deliver(ACE_Message_Block* data) const
{
    static Logger::ProcLog log("deliver", Log());
    LOGINFO << std::endl;

    if (entries_.empty()) {
        data->release();
        return true;
    }

    auto ok = true;
    if (entries_.size() > 1) {
        // The goal here is to minimize the number of 'duplicate' calls against data, even if they are
        // lightweight. We use tmp to keep track of any unused duplicate message, being careful to free it if it
        // is not used at the end of the loop.
        //
        ACE_Message_Block* tmp = 0;
        for (size_t index = entries_.size() - 1; index; --index) {
            // Perform a delivery?
            //
            if (!entries_[index].task_->isUsingData()) continue;

            // Need new copy?
            //
            if (!tmp) tmp = data->duplicate();

            // Embed the recipient's input channel index into the message block so that it can determine where
            // it came from (all put() operations use the same message queue)
            //
            tmp->msg_priority(entries_[index].channelIndex_);
            if (entries_[index].task_->put(tmp, 0) == 0) {
                tmp = 0; // put() OK
            } else {
                ok = false; // put() failed
            }
        }

        // It is possible that we might have a non-NULL tmp if the last put() call above failed.
        //
        if (tmp) tmp->release();
    }

    // Finally, use the original message for the first recipient.
    //
    if (entries_[0].task_->isUsingData()) {
        // Embed the recipient's input channel index into the message block so that it can determine where it
        // came from (all put() operations use the same message queue)
        //
        data->msg_priority(entries_[0].channelIndex_);
        if (entries_[0].task_->put(data, 0) == -1) {
            // put() failed
            //
            data->release();
            ok = false;
        }
    } else {
        data->release();
    }

    return ok;
}

bool
RecipientList::areAnyTasksUsingData() const
{
    // Walk the list of registered recipients and find the first that is wanting data.
    //
    for (size_t index = 0; index < entries_.size(); ++index) {
        if (entries_[index].task_->isUsingData()) return true;
    }

    return false;
}
