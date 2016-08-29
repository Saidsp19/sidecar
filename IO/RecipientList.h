#ifndef SIDECAR_IO_RECIPIENT_LIST_H // -*- C++ -*-
#define SIDECAR_IO_RECIPIENT_LIST_H

#include <vector>

#include "IO/Printable.h"

class ACE_Message_Block;

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

class Task;

/** Maintain a shared list of the task IDs and their input channels which should receive a message. When a Task
    object emits a message, it adds a set of recipients to the message via MessageManager::setRecipients(). The
    recipient list contains a 2-tuple for each recipent that contains the unique task ID of the Task object to
    receive the next message, and the index of the Task's channel to deliver to.

    The deliver() method distributes ACE_Message_Block message objects to registred recipients. However, it only
    deliver a message to if the recipient desired data.
*/
class RecipientList : public Printable<RecipientList>
{
public:

    using TaskRef = boost::shared_ptr<Task>;
    
    /** Log device for RecipientList messages

        \return Log device
    */
    static Logger::Log& Log();

    /** Default constructor. Creates an empty vector of Entry objects.
     */
    RecipientList();

    /** Adds a recipient to this list. NOTE: there is no check for duplicate entries (assumed done elsewhere)

	\param taskIndex unique recipient index

	\param channelIndex input channel index of task
    */
    void add(const TaskRef& task, size_t channelIndex);

    /** Obtain the number of Entry objects.

        \return 
    */
    size_t size() const { return entries_.size(); }

    /** Distribute a message to each registered recipient Task object.

        \param data the message to deliver

        \return false if any delivery failed
    */
    bool deliver(ACE_Message_Block* data) const;

    /** Print out the task/channel entries to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& print(std::ostream& os) const;

    /** Determine if there are any recipients actively processing data. If so, then deliver() will distribute
        data. Otherewise, it does nothing.

        \return true if at least one recipient wants data
    */
    bool areAnyTasksUsingData() const;

private:

    /** 3-tuple of a Task index and an index of one of its Channel objects.
     */
    struct Entry {
	Entry(const TaskRef& task, size_t channelIndex) : task_(task), channelIndex_(channelIndex) {}
	TaskRef task_;
	size_t channelIndex_;
    };

    using Container = std::vector<Entry>;

    /** Share the container among RecipientList copies.
     */
    Container entries_;
};

}} // namespaces

/** \file
 */

#endif
