#ifndef SIDECAR_MESSAGES_MESSAGEMANAGER_H // -*- C++ -*-
#define SIDECAR_MESSAGES_MESSAGEMANAGER_H

#include "ace/Message_Block.h"
#include "boost/shared_ptr.hpp"

#include "IO/ControlMessage.h"
#include "IO/Decoder.h"
#include "Messages/Header.h"
#include "Utils/Pool.h"
#include "Utils/Utils.h"
#include "XMLRPC/XmlRpcValue.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

/** Manager class for all message objects in the SideCar system. The ACE framework uses ACE_Message_Block
    objects to pass data from one ACE_Task to another. For native SideCar C++ objects, one would like to not
    have to create an ACE_Message_Block from them in order to use this infrastructure. The approach with the
    MessageManager is to properly record when an ACE_Message_Block contains encoded (marshalled) data, and when
    it contains a reference to a native SideCar object.

    When the manager receives an IO reader in its constructor, the manager creates a new ACE_Message_Block of
    type kMetaDataType and then links the readers' encoded data block to it. The new block contains a MetaData
    structure which holds the channel ID and an unset reference pointer for Messages::Header objects. If in the
    future, the manager receives a native SideCar message object derived from Messages::Header, it releases the
    encoded data block.

    The manager provides marshalling facilities that take a stored native SideCar mesasge object and encodes it
    into the common data representation (CDR) format. The call getEncoded() returns the result of this process.
    The results of the encoding are cached so that subsequent getEncoded() calls just return the result of the
    original encoding.

    The MessageManager uses three custom allocators to provide fast allocation and deallocation of
    ACE_Message_Block, ACE_Data_Block, and MessageManager::MetaData objects. These custom allocators are based
    on the Utils::Pool class. The MessageManager class method GetAllocationStats() returns a snapshot of the
    allocation statistics for all of the custom allocators.
*/
class MessageManager : public Utils::Uncopyable
{
public:
    using Ref = boost::shared_ptr<MessageManager>;

    /** Definition of the various message types supported by the MessageManager.
     */
    enum MessageTypes {

        /** Raw, non-native data, such as messages from VME or TSPI servers.
         */
        kRawData = ACE_Message_Block::MB_DATA,

        /** Native message data (eg PRIMessage)
         */
        kMetaData,

        /** Control message. NOTE: must be the last message type, since MessageManager adds the
	    ControlMessage::Type value of the message to this value (see IsControlMessage() and
	    GetControlMessageType())
        */
        kControl = ACE_Message_Block::MB_USER
    };

    /** Internal class that contains the channel ID the message belongs to and a reference to a SideCar message
        object if one was set.
    */
    struct MetaData
    {
        /** Constructor.
         */
        MetaData() : native() {}

	/** Shared reference to a native message object, one that has either been decoded from the network or
	    file, or one that was given to an MessageManager constructor.
	*/
        Messages::Header::Ref native;

	/** Number of bytes represented by the contents of the MetaData. For encoded messages given to the
	    MessageManager, this is the value from ACE_Message_Block::total_length(). For native messages, this
	    is the value from the Messages::Header::getSize() virtual method.
	*/
        size_t size;
    };

    /** Obtain the log device for MessageManager objects

        \return log device
    */
    static Logger::Log& Log();

    /** Collection of statistics that describe how much memory is allocated by the Utils::Pool objects used
	internally by MessageManager instances.
    */
    struct AllocationStats {

	/** Stats for ACE_Message_Block objects created by MessageManager
         */
	Utils::Pool::AllocationStats messageBlocks;

	/** Stats for ACE_Data_Block objects created by MessageManager
         */
	Utils::Pool::AllocationStats dataBlocks;

	/** Stats for MetaData objects created by MessageManager
         */
	Utils::Pool::AllocationStats metaData;
    };

    /** Class method that returns information about memory allocation performed by internal MesageManager memory
        pools.

        \return AllocationStats object
    */
    static AllocationStats GetAllocationStats();

    /** Create a new ACE_Message_Block with a given capacity. The block and its underlying ACE_Data_Block come
        from the custom allocators defined for MessageManager, and the block uses a mutex locking strategy to
        protect the resulting message block from multithreaded access/change.

        \param size initial capacity for the block

        \return new ACE_Message_Block object
    */
    static ACE_Message_Block* MakeMessageBlock(size_t size, int type = kRawData);

    /** Create a new ACE_Message_Block with the type kStateChangeType. These messages signal service threads to
        change their processing state.

        \return new ACE_Message_Block object
    */
    static ACE_Message_Block* MakeControlMessage(ControlMessage::Type type, size_t size);

    /** Obtain the message type found in a data block.

        \param data message block to query

        \return message type
    */
    static int GetMessageType(ACE_Message_Block* data) { return data->msg_type(); }

    /** Determine if the given data block is a raw, non-native message.

        \param data message block to query

        \return true if so
    */
    static bool IsRawMessage(ACE_Message_Block* data) { return GetMessageType(data) == kRawData; }

    /** Determine if the given data block contains a native message.

        \param data message block to query

        \return true if so
    */
    static bool IsDataMessage(ACE_Message_Block* data) { return GetMessageType(data) == kMetaData; }

    /** Determine if the given data block contains a control message.

        \param data message block to query

        \return true if so
    */
    static bool IsControlMessage(ACE_Message_Block* data) { return GetMessageType(data) >= kControl; }

    /** Obtain the ControlMessage::Type value from a given data block. Only valid if IsControlMessage(data) is
        true.

        \param data message block to query

        \return ControlMessage::Type value
    */
    static ControlMessage::Type GetControlMessageType(ACE_Message_Block* data)
        {
            return IsControlMessage(data) ? ControlMessage::Type(data->msg_type() - kControl) :
                ControlMessage::kInvalid;
        }

    /** Constructor that takes raw data from an ACE_Message_Block object. Takes ownership of the data block.

        \param encoded object containing the raw message data

        \param metaTypeInfo message type of the encoded data. If NULL, then the
        type will be obtained directly from the message data.
    */
    MessageManager(ACE_Message_Block* data, const Messages::MetaTypeInfo* metaTypeInfo = 0);

    /** Constructor for SideCar messages. Increases the reference count of the given message by one.

        \param native reference to a SideCar message to manage

        \param cid channel ID to assign
    */
    MessageManager(const Messages::Header::Ref& native);

    /** Destructor. Releases the underlying ACE_Message_Block object.
     */
    ~MessageManager();

    /** Determine if there is a message held by this MessageManager object.

        \return true if so
    */
    bool isValid() const { return data_ && metaData_; }

    /** Obtain the raw message type of the held data block. Note that this value should match one of the values
        defined in MessageTypes.

        \return ACE_Message_Block::ACE_Message_Type value.
    */
    ACE_Message_Block::ACE_Message_Type getMessageType() const { return data_->msg_type(); }

    /** Determine if the held data block is non-native.

        \return true if so
    */
    bool hasRawData() const { return IsRawMessage(data_); }

    /** Determine if the held data block contains a native SideCar message, in either raw or encoded form.

        \return true if so
    */
    bool hasMetaData() const { return IsDataMessage(data_); }

    /** Determine if the held data block contains a control message.

        \return true if so
    */
    bool hasControlMessage() const { return IsControlMessage(data_); }

    /** Obtain the ControlMessage::Type value for the held data block. Only valid if hasControlMessage() returns
        true.

        \return ControlMessage::Type value
    */
    ControlMessage::Type getControlMessageType() const { return GetControlMessageType(data_); }

    /** Determine if held data message is a ParametersChangeRequest control message.

        \return true if so
    */
    bool isParametersChangeRequest() const
        { return getControlMessageType() == ControlMessage::kParametersChange; }

    /** Determine if held data message is a ProcessingStateChangeRequest control message.

        \return true if so
    */
    bool isProcessingStateChangeRequest() const
        { return getControlMessageType() == ControlMessage::kProcessingStateChange; }

    /** Determine if held data message is a RecordingStateChangeRequest control message.

        \return true if so
    */
    bool isRecordingStateChangeRequest() const
        { return getControlMessageType() == ControlMessage::kRecordingStateChange; }

    /** Determine if held data message is a ShutdownRequest control message.

        \return true if so
    */
    bool isShutdownRequest() const { return getControlMessageType() == ControlMessage::kShutdown; }

    /** Determine if there is a native SideCar message associated with the data block.

        \return true if native message
    */
    bool hasNative() const { return hasMetaData() && metaData_->native; }

    /** Determine if there is encoded (marshalled) data available. NOTE: although this is thread-safe, another
        thread could give the held message encoded data at any time. See getEncoded().

        \return true if so
    */
    bool hasEncoded() const { return hasMetaData() && data_->cont(); }

    /** Obtain the message type key for the held message.

        \return current message type key, kInvalid if not a valid message
    */
    Messages::MetaTypeInfo::Value getNativeMessageType() const
        { return hasNative() ? getNative()->getMetaTypeInfo().getKey() :
                Messages::MetaTypeInfo::Value::kInvalid; }

    /** Determine if the held message has a given message type key.

        \param key value to check for

        \return true if so
    */
    bool hasNativeMessageType(Messages::MetaTypeInfo::Value key) const { return getNativeMessageType() == key; }

    /** Obtain the number of bytes of managed data. If the data was given to the MessageManager in encoded form,
        then the value is the number of raw bytes received. Otherwise, it is the number of samples in the
        message, the value returned by Header::getSize() method.

        \return message size
    */
    size_t getMessageSize() const { return metaData_ ? metaData_->size : 0; }

    /** Obtain a shallow copy of the held ACE_Message_Block. Caller is responsible for releasing it. This is a
        very fast operation since new ACE_Message_Block objects are fetched from an allocation pool, and they
        only contain pointers and offset into an ACE_Data_Block, which is not replicated but shared among
        ACE_Message_Block instances.

	\return new ACE_Message_Block object
    */
    ACE_Message_Block* getMessage() { return data_->duplicate(); }

    /** Obtain encoded message data. Only valid if the instance holds a native SideCar mesage object.

        \return ACE_Message_Block pointer (may be NULL)
    */
    ACE_Message_Block* getEncoded() const;

    /** Obtain a shared reference to a held native message. NOTE: the reference may point to nothing if the type
        of the held message is not compatible with the type requested in the template call (or there is no
        native message held by the MessageManager)

	\param checked if true and the cast fails, throws std::bad_cast

        \return shared reference to held native SideCar message
    */
    template <typename T>
    typename boost::shared_ptr<T> getNative(bool checked = true) const
        {
	    boost::shared_ptr<T> ref(boost::dynamic_pointer_cast<T>(metaData_->native));
	    if (! ref && checked) throw std::bad_cast();
	    return ref;
	}

    /** Obtain a reference to the native SideCar message. Only valid if hasNative() returns true.

        \return shared reference to held native SideCar message
    */
    Messages::Header::Ref getNative() const { return metaData_->native; }

    /** Obtain a control message constructed from the held ACE_Message_Block object. NOTE: this is not valid
        unless hasControlMessage() is true.

        \return ControlMessage instance
    */
    template <typename T>
    T getControlMessage() const { return T(data_->duplicate()); }

private:

    /** Initialize the internal state.
     */
    void makeMetaData(ACE_Message_Block* data);

    ACE_Message_Block* data_;
    MetaData* metaData_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
