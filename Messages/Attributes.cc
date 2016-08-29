#include "Utils/IO.h"		// Contains operators << and >> for
				// std::strings
#include "Logger/Log.h"

#include "Attributes.h"

using namespace SideCar::Messages;

Attributes::DuplicateAttribute::DuplicateAttribute(const std::string& name)
    : Utils::Exception("Attributes::add")
{
    *this << ": attempt to add attribute that already exists - " << name;
}

Attributes::Attributes()
    : attributes_(new XmlRpc::XmlRpcValue::ValueStruct)
{
    ;
}

void
Attributes::add(const std::string& name, const XmlRpc::XmlRpcValue& value)
    throw(DuplicateAttribute)
{
    if (attributes_.hasMember(name)) {
	DuplicateAttribute ex(name);
	throw ex;
    }

    attributes_[name] = value;
}

ACE_InputCDR&
Attributes::load(ACE_InputCDR& cdr)
{
    std::string xml;
    cdr >> xml;
    int offset = 0;
    attributes_.fromXml(xml, &offset);
    return cdr;
}

ACE_OutputCDR&
Attributes::write(ACE_OutputCDR& cdr) const
{
    std::string xml(attributes_.toXml());
    cdr << xml;
    return cdr;
}
