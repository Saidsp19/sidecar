#ifndef SIDECAR_IO_IOTASK_H // -*- C++ -*-
#define SIDECAR_IO_IOTASK_H

#include "IO/Task.h"
#include "Messages/MetaTypeInfo.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

/** Derivative of IO::Task that holds a MetaTypeInfo object which describes the message type the task handles.

    IO::Task objects work with any message type, but those associated with I/O operations in the SideCar system
    operate on one specific type. Derived classes may either pass a pointer to a Messages::MetaTypeInfo object
    in the IOTask() constructor, or they may use the protected setMetaTypeInfoKeyName() method to install a
    Messages::MetaTypeInfo object.

    Note that this class is still abstract since it does not define the IO::Task::deliverDataMessage() method.
*/
class IOTask : public Task
{
public:

    static Logger::Log& Log();

    /** Obtain the MetaTypeInfo object registered with the task.
        
        \return MetaTypeInfo reference
    */
    const Messages::MetaTypeInfo* getMetaTypeInfo() const { return metaTypeInfo_; }

    /** Obtain the message type name.
        
        \return type name
    */
    const std::string& getMetaTypeInfoKeyName() const { return metaTypeInfo_->getName(); }

    /** Obtain the message type key.
        
        \return type key
    */
    Messages::MetaTypeInfo::Value getMetaTypeInfoKey() const { return metaTypeInfo_->getKey(); }

    /** Obtain the loader for the message type.
        
        \return Loader object
    */
    Messages::MetaTypeInfo::CDRLoader getMetaTypeInfoCDRLoader() const { return metaTypeInfo_->getCDRLoader(); }

    /** Obtain the loader for the message type.
        
        \return Loader object
    */
    Messages::MetaTypeInfo::XMLLoader getMetaTypeInfoXMLLoader() const { return metaTypeInfo_->getXMLLoader(); }

    /** Notification that a valid connection to a data source/sink has been made. Derived classes must invoke
        this when there is a valid connection to a remove data source.
     */
    virtual void establishedConnection();

    /** Acquire a message from an external source. The data exists in a raw, CDR-encoded form, not as a SideCar
        message. Decodes the raw message data, creating a new SideCar message based on Messages::Header. Next,
        it attaches whatever recipients are defined for this task, and then invokes the Task::put() method to
        handle delivery and processing of the message.
        
        \param data message to acquire
    */
    virtual void acquireExternalMessage(ACE_Message_Block* data);

protected:

    /** Constructor for derived classes.
        
        \param metaType pointer to Messages::MetaTypeInfo object that describes the messages that this task
        handles. May be NULL.
    */
    IOTask(const Messages::MetaTypeInfo* metaType = 0) : Task(), metaTypeInfo_(metaType) {}

    /** Install the MetaTypeInfo object to use.
        
        \param keyName name of the message type to install
    */
    void setMetaTypeInfoKeyName(const std::string& keyName)
        { metaTypeInfo_ = Messages::MetaTypeInfo::Find(keyName); }

private:
    const Messages::MetaTypeInfo* metaTypeInfo_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
