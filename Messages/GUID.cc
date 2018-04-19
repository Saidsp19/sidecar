#include <sstream>
#include <unistd.h>

#include "Logger/Log.h"

#include "GUID.h"

using namespace SideCar::Messages;

Logger::Log&
GUID::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.GUID");
    return log_;
}

TLoaderRegistry<GUID>::VersionedLoaderVector
GUID::DefineLoaders()
{
    TLoaderRegistry<GUID>::VersionedLoaderVector loaders;
    loaders.push_back(TLoaderRegistry<GUID>::VersionedLoader(1, &GUID::LoadV1));
    loaders.push_back(TLoaderRegistry<GUID>::VersionedLoader(2, &GUID::LoadV2));
    loaders.push_back(TLoaderRegistry<GUID>::VersionedLoader(3, &GUID::LoadV3));
    return loaders;
}

TLoaderRegistry<GUID> GUID::loaderRegistry_(GUID::DefineLoaders());

GUID::GUID() :
    producer_(""), messageTypeKey_(MetaTypeInfo::Value::kInvalid), messageSequenceNumber_(0), representation_("")
{
    static Logger::ProcLog log("GUID(0)", Log());
    LOGTIN << "messageSequenceNumber: " << messageSequenceNumber_ << std::endl;
}

GUID::GUID(const std::string& producer, const MetaTypeInfo& metaTypeInfo) :
    producer_(producer), messageTypeKey_(metaTypeInfo.getKey()),
    messageSequenceNumber_(metaTypeInfo.getNextSequenceNumber()), representation_("")
{
    static Logger::ProcLog log("GUID(1)", Log());
    LOGTIN << "messageSequenceNumber: " << messageSequenceNumber_ << std::endl;
}

GUID::GUID(const std::string& producer, const MetaTypeInfo& metaTypeInfo, MetaTypeInfo::SequenceType sequenceNumber) :
    producer_(producer), messageTypeKey_(metaTypeInfo.getKey()), messageSequenceNumber_(sequenceNumber),
    representation_("")
{
    static Logger::ProcLog log("GUID(2)", Log());
    LOGTIN << "messageSequenceNumber: " << messageSequenceNumber_ << std::endl;
}

const std::string&
GUID::getSequenceKey() const
{
    if (!sequenceKey_.size()) {
        std::ostringstream os;
        os << producer_ << '/' << MetaTypeInfo::GetValueValue(messageTypeKey_);
        sequenceKey_ = os.str();
    }
    return sequenceKey_;
}

const std::string&
GUID::getRepresentation() const
{
    if (!representation_.size()) {
        std::ostringstream os;
        os << producer_ << '/' << MetaTypeInfo::GetValueValue(messageTypeKey_);
        sequenceKey_ = os.str();
        os << '/' << messageSequenceNumber_;
        representation_ = os.str();
    }
    return representation_;
}

ACE_InputCDR&
GUID::load(ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("load", Log());
    LOGDEBUG << "currentVersion: " << loaderRegistry_.getCurrentVersion() << std::endl;
    return loaderRegistry_.load(this, cdr);
}

ACE_InputCDR&
GUID::LoadV1(GUID* obj, ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("LoadV1", Log());
    LOGDEBUG << std::endl;
    cdr >> obj->producer_;
    cdr >> obj->messageTypeKey_;
    cdr >> obj->messageSequenceNumber_;
    cdr >> obj->representation_;
    return cdr;
}

ACE_InputCDR&
GUID::LoadV2(GUID* obj, ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("LoadV2", Log());
    LOGINFO << "BEGIN" << std::endl;
    cdr >> obj->producer_;
    uint16_t u16;
    cdr >> u16;
    obj->messageTypeKey_ = MetaTypeInfo::Value(u16);
    cdr >> obj->messageSequenceNumber_;
    cdr >> obj->representation_;
    LOGINFO << "END" << std::endl;
    return cdr;
}

ACE_InputCDR&
GUID::LoadV3(GUID* obj, ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("LoadV3", Log());
    LOGTIN << std::endl;
    cdr >> obj->producer_;
    LOGDEBUG << "producer: " << obj->producer_ << std::endl;
    uint16_t u16;
    cdr >> u16;
    obj->messageTypeKey_ = MetaTypeInfo::Value(u16);
    LOGDEBUG << "messageTypeKey: " << u16 << std::endl;
    cdr >> obj->messageSequenceNumber_;
    LOGDEBUG << "messageSequenceNumber: " << obj->messageSequenceNumber_ << std::endl;
    LOGTOUT << std::endl;
    return cdr;
}

ACE_OutputCDR&
GUID::write(ACE_OutputCDR& cdr) const
{
    static Logger::ProcLog log("write", Log());
    LOGTIN << "version: " << loaderRegistry_.getCurrentVersion() << std::endl;
    cdr << loaderRegistry_.getCurrentVersion();
    cdr << producer_;
    uint16_t u16 = static_cast<uint16_t>(messageTypeKey_);
    LOGDEBUG << "messageTypeKey: " << u16 << std::endl;
    cdr << u16;
    cdr << messageSequenceNumber_;
    LOGTOUT << std::endl;
    return cdr;
}

std::ostream&
GUID::print(std::ostream& os) const
{
    return os << getRepresentation();
}
