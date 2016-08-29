#include <cmath>

#include "Logger/Log.h"
#include "Utils/Utils.h"

#include "BugPlot.h"
#include "XmlStreamReader.h"

using namespace SideCar::Messages;

MetaTypeInfo BugPlot::metaTypeInfo_(MetaTypeInfo::Value::kBugPlot, "BugPlot",
                                    &BugPlot::CDRLoader, &BugPlot::XMLLoader);

Logger::Log&
BugPlot::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.BugPlot");
    return log_;
}

const MetaTypeInfo&
BugPlot::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

BugPlot::Ref
BugPlot::Make(const std::string& producer, double when, double range,
              double azimuth, double elevation, const std::string& tag)
{
    Ref ref(new BugPlot(producer, when, range, azimuth, elevation, tag));
    return ref;
}

BugPlot::Ref
BugPlot::Make(ACE_InputCDR& cdr)
{
    Ref ref(new BugPlot);
    ref->load(cdr);
    return ref;
}

Header::Ref
BugPlot::CDRLoader(ACE_InputCDR& cdr)
{
    return Make(cdr);
}

Header::Ref
BugPlot::XMLLoader(const std::string& producer, XmlStreamReader& xsr)
{
    Ref ref(new BugPlot(producer));
    ref->loadXML(xsr);
    return ref;
}

BugPlot::BugPlot()
    : Header(GetMetaTypeInfo())
{
    ;
}

BugPlot::BugPlot(const std::string& producer)
    : Header(producer, GetMetaTypeInfo())
{
    ;
}

BugPlot::BugPlot(const std::string& producer, double when, double range,
                 double azimuth, double elevation, const std::string& tag)
    : Header(producer, GetMetaTypeInfo()), when_(when), range_(range),
      azimuth_(azimuth), elevation_(elevation), tag_(tag)
{
    Logger::ProcLog log("BugPlot", Log());
    LOGINFO << "range: " << range << " azimuth: " << azimuth << " elevation: "
	    << elevation << " tag: " << tag << std::endl;
}

ACE_InputCDR&
BugPlot::load(ACE_InputCDR& cdr)
{
    Header::load(cdr);
    cdr >> when_;
    cdr >> range_;
    cdr >> azimuth_;
    cdr >> elevation_;
    cdr >> tag_;
    return cdr;
}

ACE_OutputCDR&
BugPlot::write(ACE_OutputCDR& cdr) const
{
    Header::write(cdr);
    cdr << when_;
    cdr << range_;
    cdr << azimuth_;
    cdr << elevation_;
    cdr << tag_;
    return cdr;
}

std::ostream&
BugPlot::printData(std::ostream& os) const
{
    return os << "When: " << when_ << " Range: " << range_ << " Azimuth: "
	      << azimuth_ << " Elevation: " << elevation_ << " Tag: " << tag_;
}

std::ostream&
BugPlot::printDataXML(std::ostream& os) const
{
    return os << "<plot tag=\"" << tag_
	      << "\" when=\"" << when_
	      << "\" range=\"" << getRange()
	      << "\" azimuth=\"" << Utils::radiansToDegrees(getAzimuth())
	      << "\" elevation=\"" << getElevation()
	      << "\" />";
}

void
BugPlot::loadXML(XmlStreamReader& xsr)
{
    Header::loadXML(xsr);
    if (! xsr.readNextEntityAndValidate("plot"))
	::abort();

    when_ = xsr.getAttribute("when").toDouble();
    range_ = xsr.getAttribute("range").toDouble();
    azimuth_ = Utils::degreesToRadians(
	xsr.getAttribute("azimuth").toDouble());
    elevation_ = xsr.getAttribute("elevation").toDouble();
}
