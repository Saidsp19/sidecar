#ifndef SIDECAR_MESSAGES_PRIMESSAGE_H // -*- C++ -*-
#define SIDECAR_MESSAGES_PRIMESSAGE_H

#include <algorithm>
#include <complex>
#include <vector>

#include "boost/shared_ptr.hpp"

#include "Messages/Header.h"
#include "Messages/RadarConfig.h"
#include "Messages/VMEHeader.h"
#include "Time/TimeStamp.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Messages {

class VMEDataMessage;

/** Base class for all PRI-type messages created in the SideCar. Each message contains header information from
    the VME system, followed by the data samples.

    Previously, PRIMessage held a shared reference to a PRIMessageInfo object, which detailed radar
    configuration parameters. This is no longer the case. Radar settings are now contained in the RadarConfig
    class.
*/
class PRIMessage : public Header {
public:
    using Super = Header;
    using Ref = boost::shared_ptr<PRIMessage>;

    /** Internal collection of VME/RIU header attributes.
     */
    struct RIUInfo : public IO::CDRStreamable<RIUInfo>, public IO::Printable<RIUInfo> {
        /** Default constructor. Used when loading from a file.
         */
        RIUInfo() {}

        /** Conversion constructor. Acquire values from a VME message header.

            \param vme header to copy
        */
        RIUInfo(const VMEDataMessage& vme);

        double getRangeAt(double gate) const { return gate * rangeFactor + rangeMin; }

        /** Load values from a CDR input stream

            \param cdr stream to read from

            \return stream read from
        */
        ACE_InputCDR& load(ACE_InputCDR& cdr, int version);

        /** Write values to a CDR output stream

            \param cdr stream to write to

            \return stream written to
        */
        ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

        /** Print values to a C++ text stream

            \param os stream to write to

            \return stream written to
        */
        std::ostream& print(std::ostream& os) const;

        std::ostream& printXML(std::ostream& os) const;

        void loadXML(XmlStreamReader& xsr);

        uint32_t msgDesc;         ///< Description of the raw VME data
        uint32_t timeStamp;       ///< VME timestamp value
        uint32_t sequenceCounter; ///< VME sequence counter
        uint32_t shaftEncoding;   ///< Value of the shaft encoder at trigger
        uint32_t prfEncoding;     ///< *** This used to be northMark ***
        double irigTime;          ///< VME IRIG timestamp
        double rangeMin;          ///< Range value of the first gate
        double rangeFactor;       ///< Range change for subsequent gates
    };

    /** Obtain the log device for PRIMessage objects. The device name is "SideCar.Messages.PRIMEssage"

        \return log device
    */
    static Logger::Log& Log();

    /** Obtain a reference to the VME header attributes.

        \return RIUInfo reference
    */
    RIUInfo& getRIUInfo() { return riuInfo_; }

    uint32_t getSequenceCounter() const { return riuInfo_.sequenceCounter; }

    /** Obtain the shaft encoding value for the message. To get an azimuth value in radians, use
        getAzimuthStart(). This value should fall between 0 and RadarConfig::GetShaftEncodingMax().

        \return encoder value
    */
    uint32_t getShaftEncoding() const { return riuInfo_.shaftEncoding; }

    bool hasIRIGTime() const { return riuInfo_.msgDesc & VMEHeader::kIRIGValidMask; }

    double getIRIGTime() const { return riuInfo_.irigTime; }

    double getRangeMin() const { return riuInfo_.rangeMin; }

    double getRangeMax() const { return getRangeAt(size() - 1); }

    double getRangeFactor() const { return riuInfo_.rangeFactor; }

    double getRangeAt(double gate) const { return riuInfo_.getRangeAt(gate); }

    /** Obtain an azimuth value in radians for the PRI message, based on the shaft encoder and the north mark
        values from the VME syatem.

        \return azimuth in radians
    */
    double getAzimuthStart() const;

    /** Obtain the value of getAzimuthStart() + RadarConfig::GetBeamWidth().

        \return azimuth in radians
    */
    double getAzimuthEnd() const;

    virtual size_t size() const = 0;

    /** Write out the message values to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& printHeader(std::ostream& os) const;

    std::ostream& printDataXML(std::ostream& os) const;

    void loadXML(XmlStreamReader& xsr);

protected:
    /** Constructor used to create a new message that is derived from another one. Shares the PRIMessageInfo
        object with the copy, but everything else s unique to the new object.

        \param producer name of the task that is creating the new object

        \param metaTypeInfo message type description

        \param copy message to derive from
    */
    PRIMessage(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const PRIMessage::Ref& copy);

    /** Constructor used to create a new message.

        \param producer name of the task that is creating the new object

        \param metaTypeInfo message type description

        \param vme header info from the VME RIU message
    */
    PRIMessage(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const VMEDataMessage& vme);

    /** Constructor used to create a new message populated with data from a raw buffer.

        \param metaTypeInfo message type description
    */
    PRIMessage(const MetaTypeInfo& metaTypeInfo);

    /** Constructor used to create a new message populated with data from a raw buffer.

        \param metaTypeInfo message type description
    */
    PRIMessage(const std::string& producer, const MetaTypeInfo& metaTypeInfo);

    /** Read in binary data from a CDR stream.

        \param cdr stream to read from

        \return stream read from
    */
    ACE_InputCDR& loadArray(ACE_InputCDR& cdr, uint32_t& count);

    /** Write out our binary data to a CDR stream.

        \param cdr stream to write to

        \return stream written to
    */
    ACE_OutputCDR& writeArray(ACE_OutputCDR& cdr, uint32_t size) const;

private:
    RIUInfo riuInfo_; ///< VME header data

    static ACE_InputCDR& LoadV1(PRIMessage* obj, ACE_InputCDR& cdr);
    static ACE_InputCDR& LoadV2(PRIMessage* obj, ACE_InputCDR& cdr);
    static ACE_InputCDR& LoadV3(PRIMessage* obj, ACE_InputCDR& cdr);
    static ACE_InputCDR& LoadV4(PRIMessage* obj, ACE_InputCDR& cdr);

    using LoaderRegistry = TLoaderRegistry<PRIMessage>;
    static LoaderRegistry::VersionedLoaderVector DefineLoaders();
    static LoaderRegistry loaderRegistry_;
};

/** Reference-counting reference to message object. Derived from boost::shared_ptr to allow [] indexing.
 */
template <typename _T>
class TPRIMessageRef : public boost::shared_ptr<_T> {
public:
    using HeldType = _T;
    using Base = boost::shared_ptr<_T>;
    using DatumType = typename _T::DatumType;

    /** Default constructor.
     */
    TPRIMessageRef() : Base() {}

    /** Constructor that takes ownership of a TPRIMessage object

        \param self object to acquire
    */
    TPRIMessageRef(_T* self) : Base(self) {}

    /** Conversion constructor for boost::shared_ptr objects.

        \param r shared ptr to share
    */
    TPRIMessageRef(Base const& r) : Base(r) {}

    /** Obtain a read-only reference to the item at a given position

        \param index position to dereference

        \return data reference
    */
    const DatumType& operator[](size_t index) const { return Base::get()->operator[](index); }

    /** Obtain a reference to the item at a given position

        \param index position to dereference

        \return data reference
    */
    DatumType& operator[](size_t index) { return Base::get()->operator[](index); }

    /** Assignment operator.

        \param rhs object to copy from

        \return reference to self
    */
    TPRIMessageRef& operator=(TPRIMessageRef const& rhs)
    {
        Base::operator=(rhs);
        return *this;
    }
};

/** Templated version of the PRIMessage class. Contains type-specific implementations and overrides. The sole
    template parameter is a type trait definition structure, which must contain the following definitions:

    - Type -- data type for a single sample (eg. int16, float)
    - Reader -- function to call to read in sample data from a CDR input stream
    - Writer -- function to call to write sample data to a CDR output stream
    - Printer -- function to call to print sample data to a C++ text stream

    Since nearly all of the type-specific functionality is contained in the
    type traits structure, TPRIMessage classes are very light-weight in terms
    of code bloat.
*/
template <typename _V>
class TPRIMessage : public PRIMessage {
public:
    using TraitsType = _V;
    using DatumType = typename _V::Type;
    using value_type = typename _V::Type; // for stl::vector compatibility
    using Self = TPRIMessage<_V>;
    using Ref = boost::shared_ptr<Self>;
    using Super = PRIMessage;
    using Container = std::vector<DatumType>;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;

    /** Obtain a writeable reference to the underlying sample data container. Should be used with caution.

        \return Container reference
    */
    Container& getData() { return data_; }

    const Container& getData() const { return data_; }

    double getRangeAt(double gate) const { return PRIMessage::getRangeAt(gate); }

    /** Obtain the range of a particular sample

        \param pos iterator pointing to the sample to query

        \return range in kilometers
    */
    double getRangeAt(const_iterator pos) const { return PRIMessage::getRangeAt(pos - data_.begin()); }

    /** Allocate enough space in the sample container to hold a certain number of samples. Simply forwards to
        the container's reserve() method.

        \param size space to reserve
    */
    void reserve(size_t size) { data_.reserve(size); }

    /** Obtain the number of data elements in the message.

        \return count
    */
    size_t size() const { return data_.size(); }

    /** Change the size of the container to hold a given number of values.

        \param size new size of the container

        \param init value to use for new values added when expanding the
        container
    */
    void resize(size_t size, DatumType init = DatumType()) { data_.resize(size, init); }

    /** Determine if the message has any data at all.

        \return true if empty
    */
    bool empty() const { return data_.empty(); }

    /** Append a sample value to the end of the message

        \param datum value to append
    */
    void push_back(const DatumType& value) { data_.push_back(value); }

    /** Index operator to access samples using array notation.

        \param index sample to obtain

        \return reference to indexed sample
    */
    DatumType& operator[](size_t index) { return data_[index]; }

    /** Read-only index operator to access samples using array notation.

        \param index sample to obtain

        \return read-only reference to indexed sample
    */
    const DatumType& operator[](size_t index) const { return data_[index]; }

    /** Obtain read-only iterator to the first sample value in the message.

        \return read-only iterator
    */
    const_iterator begin() const { return data_.begin(); }

    /** Obtain read-only iterator to the last + 1 sample value in the message.

        \return read-only iterator
    */
    const_iterator end() const { return data_.end(); }

    /** Obtain writable iterator to the first sample value in the message.

        \return writable iterator
    */
    iterator begin() { return data_.begin(); }

    /** Obtain writable iterator to the last + 1 sample value in the message.

        \return writable iterator
    */
    iterator end() { return data_.end(); }

    size_t getSize() const { return data_.size() * sizeof(DatumType) + Header::getSize() + sizeof(RIUInfo); }

    /** Read in binary data from a CDR stream.

        \param cdr stream to read from

        \return stream read from
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr)
    {
        uint32_t count;
        loadArray(cdr, count);
        _V::Reader(cdr, count, data_);
        return cdr;
    }

    /** Write out our binary data to a CDR stream.

        \param cdr stream to write to

        \return stream written to
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const
    {
        writeArray(cdr, size());
        _V::Writer(cdr, data_);
        return cdr;
    }

    /** Write out the message values to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& printData(std::ostream& os) const
    {
        _V::Printer(os, data_);
        return os;
    }

    void loadXML(XmlStreamReader& xsr)
    {
        Super::loadXML(xsr);
        _V::ReaderXML(xsr, data_);
    }

    /** Write out the message values to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& printDataXML(std::ostream& os) const
    {
        Super::printDataXML(os);
        os << "<samples count=\"" << data_.size() << "\">";
        _V::PrinterXML(os, data_);
        return os << "</samples>";
    }

protected:
    /** Constructor for new PRI messages created from a VME RIU message.

        \param producer algorithm/task creating the message

        \param metaTypeInfo type specification for the message

        \param vme VME header describing the data

        \param size number of samples to reserve in the container
    */
    TPRIMessage(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const VMEDataMessage& vme, size_t size) :
        Super(producer, metaTypeInfo, vme), data_()
    {
        data_.reserve(size);
    }

    TPRIMessage(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const VMEDataMessage& vme,
                const Container& data) :
        Super(producer, metaTypeInfo, vme),
        data_(data)
    {
    }

    /** Constructor for messages derived from another PRIMessage type.

        \param producer algorithm/task creating the message

        \param metaTypeInfo type specification for the message

        \param copy reference to the basis message

        \param size number of samples to reserve in the container
    */
    TPRIMessage(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const PRIMessage::Ref& copy,
                size_t size) :
        Super(producer, metaTypeInfo, copy),
        data_()
    {
        data_.reserve(size);
    }

    /** Constructor for messages loaded from a CDR stream.

        \param producer algorithm/task creating the message
    */
    TPRIMessage(const MetaTypeInfo& metaTypeInfo) : Super(metaTypeInfo), data_() {}

    /** Constructor for messages loaded from a CDR stream.

        \param producer algorithm/task creating the message
    */
    TPRIMessage(const std::string& producer, const MetaTypeInfo& metaTypeInfo) : Super(producer, metaTypeInfo), data_()
    {
    }

    Container data_; ///< Collection of PRIDatum values
};

/** Definitions for various sample data types. These traits may be used as a parameter to the TPRIMessage
    template defined above.
*/
namespace Traits {

/** Template function that will print out sample values from a vector onto a C++ output stream. The number of
    elements placed on a line is set by the kPerLine template parameter.

    \param os stream to write to

    \param data container with the data
*/
template <typename T, int kPerLine>
void
GenericPrinter(std::ostream& os, const std::vector<T>& data)
{
    size_t count = data.size();
    const T* ptr(count ? &data[0] : 0);
    os << "Size: " << count << '\n';
    while (count) {
        for (int index = 0; index < kPerLine && count; ++index, --count) os << *ptr++ << ' ';
        os << '\n';
    }
}

/** Traits for binary samples (on/off)
 */
struct Bool {
    using Type = char;
    static void Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data);
    static void Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data);
    static void Printer(std::ostream& os, const std::vector<Type>& data);
    static void ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data);
    static void PrinterXML(std::ostream& os, const std::vector<Type>& data);
};

/** Traits for samples of 16 bits.
 */
struct Int16 {
    using Type = int16_t;
    static void Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data);
    static void Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data);
    static void Printer(std::ostream& os, const std::vector<Type>& data);
    static void ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data);
    static void PrinterXML(std::ostream& os, const std::vector<Type>& data);
};

struct ComplexInt16 {
    using Type = std::complex<int16_t>;
    static void Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data);
    static void Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data);
    static void Printer(std::ostream& os, const std::vector<Type>& data);
    static void ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data);
    static void PrinterXML(std::ostream& os, const std::vector<Type>& data);
};

/** Traits for samples of 32 bits.
 */
struct Int32 {
    using Type = int32_t;
    static void Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data);
    static void Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data);
    static void Printer(std::ostream& os, const std::vector<Type>& data);
    static void ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data);
    static void PrinterXML(std::ostream& os, const std::vector<Type>& data);
};

/** Traits for samples of IEEE single-precision floating-point values
 */
struct Float {
    using Type = float;
    static void Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data);
    static void Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data);
    static void Printer(std::ostream& os, const std::vector<Type>& data);
    static void ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data);
    static void PrinterXML(std::ostream& os, const std::vector<Type>& data);
};

/** Traits for samples of IEEE double-precision floating-point values
 */
struct Double {
    using Type = double;
    static void Reader(ACE_InputCDR& cdr, size_t size, std::vector<Type>& data);
    static void Writer(ACE_OutputCDR& cdr, const std::vector<Type>& data);
    static void Printer(std::ostream& os, const std::vector<Type>& data);
    static void ReaderXML(XmlStreamReader& xsr, std::vector<Type>& data);
    static void PrinterXML(std::ostream& os, const std::vector<Type>& data);
};

} // end namespace Traits

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
