#ifndef SIDECAR_IO_CHANNEL_H // -*- C++ -*-
#define SIDECAR_IO_CHANNEL_H

#include "boost/shared_ptr.hpp"

#include "IO/RecipientList.h"
#include "Messages/MetaTypeInfo.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

class Task;

/** Representation of a named and typed data channel. Channels represent typed-connections between IO::Task
    objects. Channels are one-way, from sender to one or more receipients. Use addRecipient() to add a new
    connection to another IO::Task object.

    All recipient connections have an index value associated with them, which is the index into the recipient's
    list of input channels. This value is conveyed to the receiving Task by storing it in the priority slot of
    the ACE_Message_Block of the containing the message. The code for RecipientList::deliver() describes this in
    detail.
*/
class Channel 
{
public:
    using TaskRef = boost::shared_ptr<Task>;

    static Logger::Log& Log();

    /** Constructor.

        \param name assigned to the channel (for diagnostics and linking)

        \param typeName name of the message type the channel supports
    */
    Channel(const std::string& name, const std::string& typeName)
	: name_(name), metaTypeInfo_(Messages::MetaTypeInfo::Find(typeName)), recipients_(new RecipientList) {}

    /** Constructor. Allow specification of the first recipient.

        \param name name of the channel

        \param typeName name of the message type the channel supports

	\param taskIndex the task index for the first recipient

	\param channelIndex the channel index for the first recipient
    */
    Channel(const std::string& name, const std::string& typeName, const TaskRef& task, size_t channelIndex = 0)
	: name_(name), metaTypeInfo_(Messages::MetaTypeInfo::Find(typeName)), recipients_(new RecipientList)
	{
            addRecipient(task, channelIndex);
        }

    /** Obtain the name of the channel. A channel name is defined by the task that holds the Channel object.

        \return channel name
    */
    const std::string& getName() const { return name_; }

    const Messages::MetaTypeInfo* getMetaTypeInfo() const { return metaTypeInfo_; }

    /** Obtain the name of the data type configured for the channel
        
        \return data type name
    */
    const std::string& getTypeName() const { return metaTypeInfo_->getName(); }

    /** Obtain the unique key of the data type configured for the channel.

        \return type key
    */
    Messages::MetaTypeInfo::Value getTypeKey() const { return metaTypeInfo_->getKey(); }

    /** Add a message recipient to the list of recipients associated with the channel.
        
        \param taskIndex unique task identifier of the recipient

        \param channelIndex recipient's channel index for this output channel
    */
    void addRecipient(const TaskRef& task, size_t channelIndex) { recipients_->add(task, channelIndex); }

    /** Set the Task object that sends data over this channel.

        \param sender Task that sends data over the channel
    */
    void setSender(const TaskRef& sender) { sender_ = sender; }
    
    /** Obtain the Task object that sends data over this channel.
        
        \return Task reference (may be empty if never set)
    */
    const TaskRef& getSender() const { return sender_; }

    /** Deliver a message over the channel to the registered recipients.

        \param data the message to deliver

        \return true if successful
    */
    bool deliver(ACE_Message_Block* data) const { return recipients_->deliver(data); }

    /** Invoke Task::setUsingData() for the sender assigned to this channel.

        \param value the new value to use
    */
    void updateSenderUsingData(bool value) const;

    /** Determine if there are any downstream recipients that demand data from the channel's sender.
        
        \return true if so
    */
    bool areAnyRecipientsUsingData() const { return recipients_->areAnyTasksUsingData(); }

private:
    TaskRef sender_;
    std::string name_;
    const Messages::MetaTypeInfo* metaTypeInfo_;
    boost::shared_ptr<RecipientList> recipients_;
};

/** Collection of Channel objects. Note that the collection holds actual Channel objects, not their pointers.
    Currently, the SideCar software only supports adding Channel objects, and most Tasks only have one input or
    one output, so most ChannelVector objects are very small. If these operating characteristics change, we
    should reconsider the types held in a ChannelVector, possibly holding a boost::shared_ptr<Channel> instead.
*/
class ChannelVector
{
public:
    using Container = std::vector<Channel>;

    static Logger::Log& Log();

    /** Default constructor.
     */
    ChannelVector() : container_() {}

    /** Obtain the number of channels in the container.

        \return channel count
    */
    size_t size() const { return container_.size(); }

    /** Determine if the container is empty.

        \return true if so
    */
    bool empty() const { return container_.empty(); }

    /** Add a new Channel object to the end of the collection.

        \param channel object to append
    */
    void add(const Channel& channel);

    /** Obtain the Channel object at a given index of the container.

        \param index location to fetch

        \return Channel reference
    */
    const Channel& getChannel(size_t index) const { return container_[index]; }

    /** Obtain the index of the first Channel object that has a given name.

        \param name the Channel name to look for

        \return the index of the Channel, or size() if none found.
    */
    size_t findName(const std::string& name) const;

    /** Obtain the Channel object at a given index of the container.

        \param index location to fetch

        \return Channel reference
    */
    const Channel& getChannel(const std::string& name) const { return container_[findName(name)]; }

    /** Determine if a given name exists in the container

        \param name the channel name to look for

        \return true if so
    */
    bool containsChannel(const std::string& name) const { return findName(name) != container_.size(); }

    /** Invoke Channel::updateSenderUsingData() for each Channel in this container.

        \param value the new value to use
    */
    void updateSendersUsingData(bool value);

    /** Determine if there are any downstream recipients that demand data from the channel's sender.

        \note Stops checking after any Channel returns true.

        \return true if so
    */
    bool areAnyRecipientsUsingData() const;

private:
    Container container_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
