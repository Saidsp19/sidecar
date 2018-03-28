#include <algorithm> // for std::transform
#include <cmath>
#include <functional> // for std::bind* and std::mem_fun*

#include "boost/bind.hpp"
#include <iostream>
#include <limits>

#include "Logger/Log.h"
#include "Utils/Utils.h"

#include "PRIMessage.h"
#include "VMEHeader.h"
#include "XmlStreamReader.h"

using namespace SideCar;
using namespace SideCar::Messages;

Logger::Log&
PRIMessage::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.PRIMessage");
    return log_;
}

PRIMessage::LoaderRegistry::VersionedLoaderVector
PRIMessage::DefineLoaders()
{
    LoaderRegistry::VersionedLoaderVector loaders;
    loaders.push_back(LoaderRegistry::VersionedLoader(1, &PRIMessage::LoadV1));
    loaders.push_back(LoaderRegistry::VersionedLoader(2, &PRIMessage::LoadV2));
    loaders.push_back(LoaderRegistry::VersionedLoader(3, &PRIMessage::LoadV3));
    loaders.push_back(LoaderRegistry::VersionedLoader(4, &PRIMessage::LoadV4));
    return loaders;
}

PRIMessage::LoaderRegistry PRIMessage::loaderRegistry_(PRIMessage::DefineLoaders());

PRIMessage::RIUInfo::RIUInfo(const VMEDataMessage& vme) :
    msgDesc(vme.header.msgDesc), timeStamp(vme.header.timeStamp), sequenceCounter(vme.header.pri),
    shaftEncoding(vme.header.azimuth), prfEncoding(vme.header.temp1), irigTime(vme.header.irigTime),
    rangeMin(vme.rangeMin), rangeFactor(vme.rangeFactor)
{
    ;
}

ACE_InputCDR&
PRIMessage::RIUInfo::load(ACE_InputCDR& cdr, int version)
{
    cdr >> msgDesc;
    cdr >> timeStamp;
    cdr >> sequenceCounter;
    cdr >> shaftEncoding;
    cdr >> prfEncoding;
    cdr >> irigTime;
    if (version > 3) {
        cdr >> rangeMin;
        cdr >> rangeFactor;
    } else {
        rangeMin = RadarConfig::GetRangeMin_deprecated();
        rangeFactor = RadarConfig::GetRangeFactor_deprecated();
    }

    return cdr;
}

ACE_OutputCDR&
PRIMessage::RIUInfo::write(ACE_OutputCDR& cdr) const
{
    static Logger::ProcLog log("RIUInfo.write", Log());
    LOGTIN << std::endl;

    cdr << msgDesc;
    cdr << timeStamp;
    cdr << sequenceCounter;
    cdr << shaftEncoding;
    cdr << prfEncoding;
    cdr << irigTime;
    cdr << rangeMin;
    cdr << rangeFactor;
    LOGTOUT << std::endl;
    return cdr;
}

std::ostream&
PRIMessage::RIUInfo::print(std::ostream& os) const
{
    return os << " Desc: " << std::hex << msgDesc << std::dec << " TimeStamp: " << timeStamp
              << " Seq#: " << sequenceCounter << " Shaft: " << shaftEncoding << " IRIG: " << irigTime;
}

std::ostream&
PRIMessage::RIUInfo::printXML(std::ostream& os) const
{
    return os << "<riu desc=\"" << msgDesc << "\" time=\"" << timeStamp << "\" seq=\"" << sequenceCounter
              << "\" shaft=\"" << shaftEncoding << "\" prf=\"" << prfEncoding << "\" irig=\"" << irigTime
              << "\" rangeMin=\"" << rangeMin << "\" rangeFactor=\"" << rangeFactor << "\"/>\n";
}

void
PRIMessage::RIUInfo::loadXML(XmlStreamReader& xsr)
{
    msgDesc = xsr.getAttribute("desc").toUInt();
    timeStamp = xsr.getAttribute("time").toUInt();
    sequenceCounter = xsr.getAttribute("seq").toUInt();
    shaftEncoding = xsr.getAttribute("shaft").toUInt();
    prfEncoding = xsr.getAttribute("prf").toUInt();
    irigTime = xsr.getAttribute("irig").toDouble();
    rangeMin = xsr.getAttribute("rangeMin").toDouble();
    rangeFactor = xsr.getAttribute("rangeFactor").toDouble();
}

PRIMessage::PRIMessage(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const VMEDataMessage& vme) :
    Super(producer, metaTypeInfo, Header::Ref()), riuInfo_(vme)
{
    ;
}

PRIMessage::PRIMessage(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const PRIMessage::Ref& copy) :
    Super(producer, metaTypeInfo, copy), riuInfo_(copy->getRIUInfo())
{
    ;
}

PRIMessage::PRIMessage(const std::string& producer, const MetaTypeInfo& metaTypeInfo) :
    Super(producer, metaTypeInfo), riuInfo_()
{
    ;
}

PRIMessage::PRIMessage(const MetaTypeInfo& metaTypeInfo) : Super(metaTypeInfo), riuInfo_()
{
    ;
}

ACE_InputCDR&
PRIMessage::loadArray(ACE_InputCDR& cdr, uint32_t& count)
{
    static Logger::ProcLog log("load", Log());
    Super::load(cdr);
    loaderRegistry_.load(this, cdr);
    cdr >> count;
    LOGDEBUG << "count: " << count << std::endl;
    return cdr;
}

double
PRIMessage::getAzimuthStart() const
{
    return RadarConfig::GetAzimuth(riuInfo_.shaftEncoding);
}

double
PRIMessage::getAzimuthEnd() const
{
    return Utils::normalizeRadians(RadarConfig::GetAzimuth(riuInfo_.shaftEncoding) + RadarConfig::GetBeamWidth());
}

ACE_InputCDR&
PRIMessage::LoadV1(PRIMessage* obj, ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("LoadV1", Log());
    static uint32_t priCounter = 0;
    LOGTIN << std::endl;

    float azimuthStart, azimuthEnd;
    cdr >> azimuthStart;
    cdr >> azimuthEnd;
    float rangeMin, rangeFactor;
    cdr >> rangeMin;
    cdr >> rangeFactor;

    RIUInfo& riuInfo(obj->riuInfo_);
    riuInfo.msgDesc = (VMEHeader::kPackedReal << 1) | VMEHeader::kAzimuthValidMask | VMEHeader::kPRIValidMask;
    riuInfo.timeStamp = 0;
    riuInfo.sequenceCounter = ++priCounter;
    riuInfo.shaftEncoding = static_cast<uint32_t>(azimuthStart / (2 * M_PI) * RadarConfig::GetShaftEncodingMax());
    riuInfo.prfEncoding = 0;
    riuInfo.irigTime = 0.0;
    riuInfo.rangeMin = rangeMin;
    riuInfo.rangeFactor = rangeFactor;
    return cdr;
}

ACE_InputCDR&
PRIMessage::LoadV2(PRIMessage* obj, ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("LoadV2", Log());
    static uint32_t priCounter = 0;
    LOGTIN << std::endl;

    double rangeMin, rangeFactor;
    cdr >> rangeMin;
    cdr >> rangeFactor;
    double beamWidth;
    cdr >> beamWidth;
    LOGDEBUG << "beamWidth: " << beamWidth << std::endl;
    uint16_t shaftEncoding, shaftEncodingRange, prfEncoding;
    cdr >> shaftEncoding;
    cdr >> shaftEncodingRange;
    LOGDEBUG << "shaftEncoding: " << shaftEncoding << std::endl;
    LOGDEBUG << "shaftEncodingRange: " << shaftEncodingRange << std::endl;
    cdr >> prfEncoding;

    RIUInfo& riuInfo(obj->riuInfo_);
    riuInfo.msgDesc = (VMEHeader::kPackedReal << 1) | VMEHeader::kAzimuthValidMask | VMEHeader::kPRIValidMask;
    riuInfo.timeStamp = 0;
    riuInfo.sequenceCounter = ++priCounter;
    riuInfo.shaftEncoding = shaftEncoding;
    riuInfo.prfEncoding = prfEncoding;
    riuInfo.irigTime = 0.0;
    riuInfo.rangeMin = rangeMin;
    riuInfo.rangeFactor = rangeFactor;
    LOGDEBUG << "rangeMin: " << riuInfo.rangeMin << " rangeFactor: " << riuInfo.rangeFactor << std::endl;
    return cdr;
}

ACE_InputCDR&
PRIMessage::LoadV3(PRIMessage* obj, ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("LoadV3", Log());
    LOGTIN << std::endl;
    obj->riuInfo_.load(cdr, 3);
    return cdr;
}

ACE_InputCDR&
PRIMessage::LoadV4(PRIMessage* obj, ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("LoadV4", Log());
    LOGTIN << std::endl;
    obj->riuInfo_.load(cdr, 4);
    return cdr;
}

ACE_OutputCDR&
PRIMessage::writeArray(ACE_OutputCDR& cdr, uint32_t count) const
{
    Super::write(cdr);
    cdr << loaderRegistry_.getCurrentVersion();
    cdr << riuInfo_;
    cdr << count;
    return cdr;
}

std::ostream&
PRIMessage::printHeader(std::ostream& os) const
{
    return riuInfo_.print(Super::printHeader(os));
}

std::ostream&
PRIMessage::printDataXML(std::ostream& os) const
{
    riuInfo_.printXML(os);
    return os;
}

void
PRIMessage::loadXML(XmlStreamReader& xsr)
{
    Super::loadXML(xsr);
    if (!xsr.readNextEntityAndValidate("riu")) ::abort();
    riuInfo_.loadXML(xsr);
}

static QStringList
GetSamples(XmlStreamReader& xsr)
{
    if (!xsr.readNextEntityAndValidate("samples")) ::abort();
    return xsr.getSamples();
}

template <typename T, typename U>
void
GenericReaderXML(const QStringList& values, std::vector<T>& data, U converter)
{
    std::transform(values.begin(), values.end(), std::back_inserter(data), converter);
}

template <typename T>
void
GenericPrinterXML(std::ostream& os, const std::vector<T>& data)
{
    size_t count = data.size();
    const T* ptr(count ? &data[0] : 0);
    while (count--) { os << *ptr++ << ' '; }
}

void
Traits::Bool::Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data)
{
    if (size) {
        data.resize(size);
        cdr.read_char_array(&data[0], size);
    }
}

void
Traits::Bool::Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data)
{
    if (data.size()) { cdr.write_char_array(&data[0], data.size()); }
}

void
Traits::Bool::Printer(std::ostream& os, const std::vector<Type>& data)
{
    size_t count = data.size();
    const Type* ptr(count ? &data[0] : 0);
    os << "Size: " << count << '\n';
    while (count) {
        for (int index = 0; index < 60 && count; ++index, --count) os << int(*ptr++) << ' ';
        os << '\n';
    }
}

struct ConvertQStringBool {
    bool operator()(const QString& v) const { return v.toInt() ? true : false; }
};

void
Traits::Bool::ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data)
{
    GenericReaderXML(GetSamples(xsr), data, ConvertQStringBool());
}

void
Traits::Bool::PrinterXML(std::ostream& os, const std::vector<Type>& data)
{
    size_t count = data.size();
    const Type* ptr(count ? &data[0] : 0);
    while (count--) os << int(*ptr++ ? 1 : 0) << ' ';
}

void
Traits::Int16::Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data)
{
    if (size) {
        data.resize(size);
        cdr.read_short_array(&data[0], size);
    }
}

void
Traits::Int16::Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data)
{
    if (data.size()) { cdr.write_short_array(&data[0], data.size()); }
}

void
Traits::Int16::Printer(std::ostream& os, const std::vector<Type>& data)
{
    GenericPrinter<Type, 40>(os, data);
}

struct ConvertQStringShort {
    Traits::Int16::Type operator()(const QString& v) const { return v.toShort(); }
};

void
Traits::Int16::ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data)
{
    GenericReaderXML(GetSamples(xsr), data, ConvertQStringShort());
}

void
Traits::Int16::PrinterXML(std::ostream& os, const std::vector<Type>& data)
{
    GenericPrinterXML<Type>(os, data);
}

void
Traits::ComplexInt16::Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data)
{
    if (size) {
        data.resize(size);
        cdr.read_short_array(reinterpret_cast<int16_t*>(&data[0]), size * 2);
    }
}

void
Traits::ComplexInt16::Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data)
{
    if (data.size()) { cdr.write_short_array(reinterpret_cast<const int16_t*>(&data[0]), data.size() * 2); }
}

void
Traits::ComplexInt16::Printer(std::ostream& os, const std::vector<Type>& data)
{
    GenericPrinter<Type, 40>(os, data);
}

struct ConvertQStringComplexInt16 {
    Traits::ComplexInt16::Type operator()(const QString& v) const
    {
        QStringList bits = v.split(',');
        return Traits::ComplexInt16::Type(bits[0].toShort(), bits[1].toShort());
    }
};

void
Traits::ComplexInt16::ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data)
{
    GenericReaderXML(GetSamples(xsr), data, ConvertQStringComplexInt16());
}

void
Traits::ComplexInt16::PrinterXML(std::ostream& os, const std::vector<Type>& data)
{
    size_t count = data.size();
    const Type* ptr(count ? &data[0] : 0);
    while (count--) {
        os << ptr->real() << ',' << ptr->imag() << ' ';
        ++ptr;
    }
}

void
Traits::Int32::Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data)
{
    data.reserve(size);
    int32_t value;
    while (size--) {
        cdr >> value;
        data.push_back(value);
    }
}

void
Traits::Int32::Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data)
{
    std::vector<Type>::const_iterator pos(data.begin());
    std::vector<Type>::const_iterator end(data.end());
    while (pos != end) {
        int32_t value(*pos++);
        cdr << value;
    }
}

void
Traits::Int32::Printer(std::ostream& os, const std::vector<Type>& data)
{
    GenericPrinter<Type, 20>(os, data);
}

struct ConvertQStringInt {
    Traits::Int32::Type operator()(const QString& v) const { return v.toInt(); }
};

void
Traits::Int32::ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data)
{
    GenericReaderXML(GetSamples(xsr), data, ConvertQStringInt());
}

void
Traits::Int32::PrinterXML(std::ostream& os, const std::vector<Type>& data)
{
    GenericPrinterXML(os, data);
}

void
Traits::Float::Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data)
{
    if (size) {
        data.resize(size);
        cdr.read_float_array(&data[0], size);
    }
}

void
Traits::Float::Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data)
{
    if (data.size()) { cdr.write_float_array(&data[0], data.size()); }
}

void
Traits::Float::Printer(std::ostream& os, const std::vector<Type>& data)
{
    GenericPrinter<Type, 20>(os, data);
}

struct ConvertQStringFloat {
    Traits::Float::Type operator()(const QString& v) const { return v.toFloat(); }
};

void
Traits::Float::ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data)
{
    GenericReaderXML(GetSamples(xsr), data, ConvertQStringFloat());
}

void
Traits::Float::PrinterXML(std::ostream& os, const std::vector<Type>& data)
{
    GenericPrinterXML<Type>(os, data);
}

void
Traits::Double::Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data)
{
    if (size) {
        data.resize(size);
        cdr.read_double_array(&data[0], size);
    }
}

void
Traits::Double::Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data)
{
    if (data.size()) { cdr.write_double_array(&data[0], data.size()); }
}

void
Traits::Double::Printer(std::ostream& os, const std::vector<Type>& data)
{
    GenericPrinter<Type, 10>(os, data);
}

struct ConvertQStringDouble {
    Traits::Double::Type operator()(const QString& v) const { return v.toDouble(); }
};

void
Traits::Double::ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data)
{
    GenericReaderXML(GetSamples(xsr), data, ConvertQStringDouble());
}

void
Traits::Double::PrinterXML(std::ostream& os, const std::vector<Type>& data)
{
    GenericPrinterXML<Type>(os, data);
}
