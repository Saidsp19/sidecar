#ifndef SIDECAR_MESSAGES_CONTROLMESSAGE_H // -*- C++ -*-
#define SIDECAR_MESSAGES_CONTROLMESSAGE_H

#include "ace/Message_Block.h"

namespace SideCar {
namespace IO {

/** Base class for all SideCar control messages. A control message affects the runtime behavior of one or more
    IO::Task objects. One cannot instantiate a ControlMessage directly.
*/
class ControlMessage {
public:
    /** Control message types. When wrapped by an ACE_Message_Block, the block will have a message type equal to
        IO::MessageManager::kControl + one of the following values. When creating new control messages, add new
        values to the end of the enumeration.
    */
    enum Type {

        kInvalid = -1,

        /** Remote parameter change message. The contents of the buffer is an XmlRpc::XmlRpcValue value,
            containing one or more name/value pairs describing new parameter settings.
        */
        kParametersChange,

        /** Remote state change message. The contents of the buffer is an int value specifying the new state.
         */
        kProcessingStateChange,

        /** Recording mode change message. The contents of the buffer is a boolean value indicating the
            recording state.
        */
        kRecordingStateChange,

        /** Shutdown a stream.
         */
        kShutdown,

        /** Clear statistics
         */
        kClearStats,

        /** Process an alarm that an Algorithm has set for itself
         */
        kTimeout,

        kNumTypes
    };

    /** Copy constructor.

        \param copy object to copy
    */
    ControlMessage(const ControlMessage& copy) : data_(copy.data_->duplicate()) {}

    /** Destructor. Decrease reference count on held data block.
     */
    virtual ~ControlMessage() { data_->release(); }

    /** Obtain the control message wrapped inside an ACE_Message_Block object. Used to transport ControlMessage
        objects from one task to another.

        \return ACE_Message_Block object
    */
    ACE_Message_Block* getWrapped() const { return data_->duplicate(); }

protected:
    ControlMessage(Type type, size_t size);
    ControlMessage(ACE_Message_Block* data) : data_(data) {}
    ACE_Message_Block* getData() const { return data_; }

private:
    ACE_Message_Block* data_;
};

} // namespace IO
} // end namespace SideCar

/** \file
 */

#endif
