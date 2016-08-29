#include <cmath>

#include "Utils/Utils.h"

#include "Extraction.h"
#include "XmlStreamReader.h"

using namespace SideCar::Messages;

MetaTypeInfo Extractions::metaTypeInfo_(MetaTypeInfo::Value::kExtractions, "Extractions",
                                        &Extractions::CDRLoader, &Extractions::XMLLoader);

Extraction::Extraction(const Time::TimeStamp& when, double range, double azimuth, double elevation)
    : when_(when), range_(range), azimuth_(azimuth), elevation_(elevation), x_(range * ::sin(azimuth)),
      y_(range * ::cos(azimuth))
{
    ;
}

Extraction::Extraction(XmlStreamReader& xsr)
{
    when_ = Time::TimeStamp::ParseSpecification(xsr.getAttribute("when").toStdString(), Time::TimeStamp::Min());
    range_ = xsr.getAttribute("range").toDouble();
    azimuth_ = Utils::degreesToRadians(xsr.getAttribute("azimuth").toDouble());
    elevation_ = xsr.getAttribute("elevation").toDouble();
}

Extraction::Extraction(ACE_InputCDR& cdr)
{
    cdr >> when_;
    cdr >> range_;
    cdr >> azimuth_;
    cdr >> elevation_;
    cdr >> x_;
    cdr >> y_;
    cdr >> attributes_;
}

ACE_OutputCDR&
Extraction::write(ACE_OutputCDR& cdr) const
{
    cdr << when_;
    cdr << range_;
    cdr << azimuth_;
    cdr << elevation_;
    cdr << x_;
    cdr << y_;
    cdr << attributes_;
    return cdr;
}

std::ostream&
Extraction::printXML(std::ostream& os) const
{
    return os << "<extraction when=\"" << when_ << "\" range=\"" << range_
	      << "\" azimuth=\"" << Utils::radiansToDegrees(azimuth_)
	      << "\" elevation=\"" << elevation_ << "\">\n";
}

const MetaTypeInfo&
Extractions::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

Extractions::Ref
Extractions::Make(const std::string& producer, const Header::Ref& basis)
{
    Ref ref(new Extractions(producer, basis));
    return ref;
}

Extractions::Ref
Extractions::Make(ACE_InputCDR& cdr)
{
    Ref ref(new Extractions);
    ref->load(cdr);
    return ref;
}

Header::Ref
Extractions::CDRLoader(ACE_InputCDR& cdr)
{
    return Make(cdr);
}

Header::Ref
Extractions::XMLLoader(const std::string& producer, XmlStreamReader& xsr)
{
    Ref ref(new Extractions(producer));
    ref->loadXML(xsr);
    return ref;
}

ACE_InputCDR&
Extractions::load(ACE_InputCDR& cdr)
{
    Header::load(cdr);
    uint32_t count;
    cdr >> count;
    data_.reserve(count);
    while (count--) {
	Extraction extraction(cdr);
	data_.push_back(extraction);
    }

    return cdr;
}

ACE_OutputCDR&
Extractions::write(ACE_OutputCDR& cdr) const
{
    Header::write(cdr);
    uint32_t count = size();
    cdr << count;
    Container::const_iterator pos = data_.begin();
    while (count--) {
	cdr << *pos++;
    }

    return cdr;
}

std::ostream&
Extractions::printData(std::ostream& os) const
{
    for (size_t index = 0; index < size(); ++index) {
	os << index << ": " << data_[index] << '\n';
    }
    return os;
}

std::ostream&
Extractions::printDataXML(std::ostream& os) const
{
    for (size_t index = 0; index < size(); ++index) {
	data_[index].printXML(os);
    }
    return os;
}

void
Extractions::loadXML(XmlStreamReader& xsr)
{
    Header::loadXML(xsr);
    while (xsr.readNextEntityAndValidate("extraction")) {
	data_.push_back(Extraction(xsr));
    }
}
