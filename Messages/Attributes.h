#ifndef SIDECAR_MESSAGES_ATTRIBUTES_H // -*- C++ -*-
#define SIDECAR_MESSAGES_ATTRIBUTES_H

#include <string>

#include "IO/CDRStreamable.h"
#include "IO/Printable.h"
#include "Utils/Exception.h"
#include "XMLRPC/XmlRpcValue.h"

namespace SideCar {
namespace Messages {

/** Collection of name/value pairs for SideCar messages. Relies on the XMLRPC library for marshalling the values
    to/from network or disk. Currently, the Extraction and BugPlot messages support attributes. If enough
    messsage classes do, it may make sense to have Header acquire an Attributes slot.
*/
class Attributes : public IO::CDRStreamable<Attributes>, public IO::Printable<Attributes> {
public:
    /** Exception thrown when a duplicate attribute name is detected.
     */
    class DuplicateAttribute : public Utils::Exception, public Utils::ExceptionInserter<DuplicateAttribute> {
    public:
        /** Constructor.

            \param name name of the parameter that rejected the value
        */
        DuplicateAttribute(const std::string& name);
    };

    /** Constructor. Initialize empty container of attributes.
     */
    Attributes();

    /** Add a new attribute to the container. Throws DuplicateAttribute exception if one already exists with the
        same name.

        \param name the attribute name

        \param value the attribute value
    */
    void add(const std::string& name, const XmlRpc::XmlRpcValue& value);

    /** Determines if there is an attribute with the given name.

        \param name the attribute name

        \return true if exists
    */
    bool contains(const std::string& name) const { return attributes_.hasMember(name); }

    /** Find an attribute with the given name.

        \param name the attribute name

        \return attribute value if found; 'invalid' XMLRPC value if not.
    */
    const XmlRpc::XmlRpcValue& find(const std::string& name) const { return attributes_[name]; }

    /** Obtain an attribute with the given name.

        \param name the attribute name

        \return attribute value if found; 'invalid' XMLRPC value if not.
    */
    const XmlRpc::XmlRpcValue& operator[](const std::string& name) const { return find(name); }

    /** Build an Attributes container from an ACE CDR stream.

        \param cdr stream to read from

        \return stream read from
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr);

    /** Write an Attributes container to an ACE CDR stream.

        \param cdr stream to write to

        \return stream written to
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

    /** Write XML representation of Attributes container to given C++ text stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& print(std::ostream& os) const { return os << attributes_.toXml(); }

private:
    XmlRpc::XmlRpcValue attributes_;
};

} // end namespace Messages
} // end namespace SideCar

#endif
