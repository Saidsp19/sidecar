#include "Logger/Log.h"

#include "Header.h"
#include "MetaTypeInfo.h"
#include "XmlStreamReader.h"

using namespace SideCar;
using namespace SideCar::Messages;

TLoaderRegistry<Header>::VersionedLoaderVector
Header::DefineLoaders()
{
    TLoaderRegistry<Header>::VersionedLoaderVector loaders;
    loaders.push_back(TLoaderRegistry<Header>::VersionedLoader(1, &Header::LoadV1));
    loaders.push_back(TLoaderRegistry<Header>::VersionedLoader(2, &Header::LoadV2));
    return loaders;
}

TLoaderRegistry<Header> Header::loaderRegistry_(Header::DefineLoaders());

Logger::Log&
Header::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.Header");
    return log_;
}

const MetaTypeInfo*
Header::GetMessageMetaTypeInfo(ACE_InputCDR& cdr)
{
    // Create a temporary Header object that we only use to obtain the message type key found in the GUID. When
    // we have that, use it to obtain the actual MetaTypeInfo object.
    //
    Header tmp(cdr);
    return MetaTypeInfo::Find(tmp.guid_.getMessageTypeKey());
}

Header::Header(ACE_InputCDR& cdr)
    : metaTypeInfo_(*MetaTypeInfo::Find(MetaTypeInfo::Value::kVideo)), guid_(), createdTimeStamp_(),
      emittedTimeStamp_(), basis_()
{
    load(cdr);
}

Header::Header(const std::string& producer, const MetaTypeInfo& metaTypeInfo)
    : metaTypeInfo_(metaTypeInfo), guid_(producer, metaTypeInfo), createdTimeStamp_(Time::TimeStamp::Now()),
      emittedTimeStamp_(), basis_()
{
    static Logger::ProcLog log("Header(0)", Log());
    LOGTIN << std::endl;
}

Header::Header(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const Ref& basis)
    : metaTypeInfo_(metaTypeInfo), guid_(producer, metaTypeInfo), createdTimeStamp_(Time::TimeStamp::Now()),
      emittedTimeStamp_(), basis_(basis)
{
    static Logger::ProcLog log("Header(1)", Log());
    LOGTIN << std::endl;
}

Header::Header(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const Ref& basis,
               MetaTypeInfo::SequenceType sequenceNumber)
    : metaTypeInfo_(metaTypeInfo), guid_(producer, metaTypeInfo, sequenceNumber),
      createdTimeStamp_(Time::TimeStamp::Now()), emittedTimeStamp_(), basis_(basis)
{
    static Logger::ProcLog log("Header(2)", Log());
    LOGTIN << std::endl;
}

Header::Header(const MetaTypeInfo& metaTypeInfo)
    : metaTypeInfo_(metaTypeInfo), guid_(), createdTimeStamp_(), emittedTimeStamp_(), basis_()
{
    static Logger::ProcLog log("Header(3)", Log());
    LOGTIN << std::endl;
}

Header::~Header()
{
    ;
}

Time::TimeStamp
Header::setCreatedTimeStamp(const Time::TimeStamp& value)
{
    Time::TimeStamp old(createdTimeStamp_);
    createdTimeStamp_ = value;
    return old;
}

Time::TimeStamp
Header::setEmittedTimeStamp(const Time::TimeStamp& value)
{
    Time::TimeStamp old(emittedTimeStamp_);
    emittedTimeStamp_ = value;
    return old;
}

ACE_InputCDR&
Header::load(ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("load", Log());
    LOGINFO << "currentVersion: " << loaderRegistry_.getCurrentVersion() << std::endl;
    return loaderRegistry_.load(this, cdr);
}

ACE_InputCDR&
Header::LoadV1(Header* obj, ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("LoadV1", Log());
    LOGDEBUG << std::endl;
    cdr >> obj->guid_;
    cdr >> obj->createdTimeStamp_;
    obj->emittedTimeStamp_ = obj->createdTimeStamp_;
    return cdr;
}

ACE_InputCDR&
Header::LoadV2(Header* obj, ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("LoadV2", Log());
    cdr >> obj->guid_;
    LOGDEBUG << "GUID: " << obj->guid_ << std::endl;
    cdr >> obj->createdTimeStamp_;
    LOGDEBUG << "createdTimeStamp: " << obj->createdTimeStamp_ << std::endl;
    cdr >> obj->emittedTimeStamp_;
    LOGDEBUG << "emittedTimeStamp: " << obj->emittedTimeStamp_ << std::endl;
    return cdr;
}

ACE_OutputCDR&
Header::write(ACE_OutputCDR& cdr) const
{
    static Logger::ProcLog log("write", Log());
    LOGDEBUG << "version: " << loaderRegistry_.getCurrentVersion() << std::endl;
    cdr << loaderRegistry_.getCurrentVersion();
    cdr << guid_;
    cdr << createdTimeStamp_;
    if (emittedTimeStamp_ == Time::TimeStamp::Min()) emittedTimeStamp_ = Time::TimeStamp::Now();
    cdr << emittedTimeStamp_;
    return cdr;
}

std::ostream&
Header::printHeader(std::ostream& os) const
{
    return os << "GUID: " << guid_.getRepresentation()
	      << " CTime: " << createdTimeStamp_
	      << " ETime: " << emittedTimeStamp_;
}

std::ostream&
Header::printData(std::ostream& os) const
{
    return os << "*** printData undefined ***";
}

std::ostream&
Header::printDataXML(std::ostream& os) const
{
    return os << "*** printDataXML undefined ***";
}

std::ostream&
Header::print(std::ostream& os) const
{
    printHeader(os) << '\n';
    printData(os) << '\n';
    return os;
}

std::ostream&
Header::printXML(std::ostream& os) const
{
    os << "<msg id=\"" << guid_.getMessageSequenceNumber()
       << "\" ctime=\"" << createdTimeStamp_
       << "\" etime=\"" << emittedTimeStamp_
       << "\">\n";
    printDataXML(os);
    return os << "</msg>";
}

void
Header::loadXML(XmlStreamReader& xsr)
{
    static Logger::ProcLog log("loadXML", Log());

    int id = xsr.getAttribute("id").toInt();
    guid_.setMessageSequenceNumber(id);
    createdTimeStamp_ = Time::TimeStamp::ParseSpecification(xsr.getAttribute("ctime").toStdString(),
                                                            Time::TimeStamp::Min());
    emittedTimeStamp_ = Time::TimeStamp::ParseSpecification(xsr.getAttribute("etime").toStdString(),
                                                            Time::TimeStamp::Min());
}
