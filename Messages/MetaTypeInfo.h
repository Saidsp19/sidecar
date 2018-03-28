#ifndef SIDECAR_MESSAGES_METATYPEINFO_H // -*- C++ -*-
#define SIDECAR_MESSAGES_METATYPEINFO_H

#include <string>
#include <vector>

#include "ace/Mutex.h"
#include "boost/shared_ptr.hpp"

#include "Utils/IO.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {
class Task;
}
namespace Messages {

class Header;
class XmlStreamReader;

/** Unique identification for message types/classes. Each unique message class must have an entry in the
    MetaTypeInfo::Value enum. It is primarily used to locate message loading procedures that unmarshal data read
    in from an external data source.
*/
class MetaTypeInfo {
public:
    class SequenceGenerator;

    using SequenceType = uint32_t;
    using HeaderRef = boost::shared_ptr<Header>;
    using TaskRef = boost::shared_ptr<IO::Task>;

    using CDRLoader = HeaderRef (*)(ACE_InputCDR& cdr);
    using XMLLoader = HeaderRef (*)(const std::string&, XmlStreamReader&);

    /** Active message types in the SideCar system. The ordering is not important, though kInvalid should always
        be first, and kUnassigned should always be last.

        *NOTE: if you add to this, add to the *END* of the list, before kUnassigned. Otherwise, past recordings
        won't play correctly.
    */
    enum class Value : uint16_t {
        kInvalid = 0, ///< Keep first
        kRawVideo, ///< Video data message emitted by the VME board
        kVideo, ///< Video data message after conversion from VME format
        kBinaryVideo, ///< Boolean data message
        kExtractions, ///< Feature extraction message
        kSegmentMessage, ///< Feature segmentation message
        kComplex, ///< IQ (complex) message
        kTSPI, ///< Message from external track (TSPI) provider
        kBugPlot, ///< Message recording a user-initiated track
        kTrack, ///< Message for an internally initiated track
        kUnassigned ///< Keep last
    };

    static auto GetValueValue(const Value& value) -> std::underlying_type_t<Value>
    {
        return static_cast<std::underlying_type_t<Value>>(value);
    }

    /** Obtain the log device for MetaTypeInfo objects

        \return Log device
    */
    static Logger::Log& Log();

    /** Obtain the MetaTypeInfo object registered under a given key. NOTE: throws an exception if not found.

        \param key the key to look for

        \return registered MetaTypeInfo object
    */
    static const MetaTypeInfo* Find(Value key);

    /** Obtain the MetaTypeInfo object registered under a given key name. NOTE: throws an exception if not
        found.

        \param keyName the key name to look for

        \return registered MetaTypeInfo object
    */
    static const MetaTypeInfo* Find(const std::string& keyName);

    /** Constructor. Registers the new MetaTypeInfo object under the given key.

        \param key the unique integral key for a message class

        \param name the unique string key for a message class

        \param loader the procedure to invoke to create a new message and load
        it from raw data
        buffer.
    */
    MetaTypeInfo(Value key, const std::string& name, CDRLoader cdrLoader, XMLLoader xmlLoader);

    /** Destructor. Unregisters the MetaTypeInfo object from the internal registry.
     */
    ~MetaTypeInfo();

    void unregister();

    /** Obtain the Value value for the message class.

        \return Value enum value
    */
    Value getKey() const;

    /** Determine if the given MetaTypeInfo key is the same as ours.

        \param key the value to compare

        \return true if so
    */
    bool isa(Value key) const { return getKey() == key; }

    /** Obtain the type name for the message class.

        \return key name
    */
    const std::string& getName() const;

    /** Obtain the next sequence number for a new message of this type.

        \return next sequence number
    */
    SequenceType getNextSequenceNumber() const;

    /** Obtain the loader for the message class.

        \return Loader procedure
    */
    CDRLoader getCDRLoader() const;

    /** Obtain the loader for the message class.

        \return Loader procedure
    */
    XMLLoader getXMLLoader() const;

    /** Less than comparison operator. Allows ordering of MetaTypeInfo objects by their type key.

        \param rhs object to compare against

        \return true if our type key is less than the other object's type key
    */
    bool operator<(const MetaTypeInfo& rhs) const;

private:
    static std::vector<std::string> InitVariantTags();

    Value key_; ///< The key value assigned to this object
    std::string name_; ///< The key name assigned to this object
    CDRLoader cdrLoader_; ///< The CDRLoader object used to read raw data
    XMLLoader xmlLoader_; ///< The XMLLoader object used to read XML data
    SequenceGenerator* sequenceGenerator_; ///< Sequence number generator

    /** Mutex for MetaType registrations_
     */
    static ACE_Mutex mutex_; ///< Mutex to protect access to registrations_

    /** Container holding registered MetaTypeInfo objects
     */
    class Registrations;
    static Registrations* registrations_;

    static std::vector<std::string> ddsVariantTags_;
};

extern bool operator>>(ACE_InputCDR& cdr, MetaTypeInfo::Value& value);

extern bool operator<<(ACE_OutputCDR& cdr, MetaTypeInfo::Value value);

inline std::ostream&
operator<<(std::ostream& os, const MetaTypeInfo::Value& value)
{
    return os << MetaTypeInfo::GetValueValue(value);
}

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
