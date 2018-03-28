#include "QtCore/QString"
#include "QtXml/QDomElement"

#include "GUI/LogUtils.h" // Pull in stream inserters for some Qt classes
#include "Utils/FilePath.h"
#include "Utils/Utils.h"

#include "RadarConfig.h"

using namespace SideCar::Messages;

std::string RadarConfig::name_ = "Default";
uint32_t RadarConfig::gateCountMax_ = 4000;
uint32_t RadarConfig::shaftEncodingMax_ = 65535;
double RadarConfig::rotationRate_ = 6.0;
double RadarConfig::rangeMin_ = 1.0;
double RadarConfig::rangeMax_ = 300.0;
double RadarConfig::rangeFactor_ = CalculateRangeFactor();
double RadarConfig::beamWidth_ = 0.001544;
double RadarConfig::latitude_ = 37.0 + 49.0 / 60.0 + 7.83477 / 3600.0;
double RadarConfig::longitude_ = -(116.0 + 31.0 / 60.0 + 53.51066 / 3600.0);
double RadarConfig::height_ = 0.0;

Logger::Log&
RadarConfig::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.RadarConfig");
    return log_;
}

const std::string&
RadarConfig::GetName()
{
    return name_;
}

uint32_t
RadarConfig::GetGateCountMax()
{
    return gateCountMax_;
}

uint32_t
RadarConfig::GetShaftEncodingMax()
{
    return shaftEncodingMax_;
}

double
RadarConfig::GetRotationRate()
{
    return rotationRate_;
}

double
RadarConfig::GetRangeMin_deprecated()
{
    return rangeMin_;
}

double
RadarConfig::GetRangeMax()
{
    return rangeMax_;
}

double
RadarConfig::GetRangeFactor_deprecated()
{
    return rangeFactor_;
}

double
RadarConfig::GetBeamWidth()
{
    return beamWidth_;
}

double
RadarConfig::CalculateRangeFactor()
{
    return (rangeMax_ - rangeMin_) / (gateCountMax_ - 1);
}

double
RadarConfig::GetSiteLongitude()
{
    return longitude_;
}

double
RadarConfig::GetSiteLatitude()
{
    return latitude_;
}

double
RadarConfig::GetSiteHeight()
{
    return height_;
}

void
RadarConfig::Load(const std::string& name, uint32_t gateCountMax, uint32_t shaftEncodingMax, double rotationRate,
                  double rangeMin, double rangeMax, double beamWidth)
{
    name_ = name;
    gateCountMax_ = gateCountMax;
    shaftEncodingMax_ = shaftEncodingMax;
    rotationRate_ = rotationRate;
    rangeMin_ = rangeMin;
    rangeMax_ = rangeMax;
    beamWidth_ = beamWidth;
    rangeFactor_ = CalculateRangeFactor();
}

bool
RadarConfig::GetEntry(const QDomElement& config, const QString& name, QString& value, QString& units)
{
    static Logger::ProcLog log("GetEntry", Log());

    QDomElement element = config.firstChildElement(name);
    if (element.isNull()) {
        LOGWARNING << "Missing field " << name << std::endl;
        return false;
    }

    units = "";
    if (element.hasAttribute("units")) { units = element.attribute("units"); }

    value = element.text();
    LOGDEBUG << "name: " << name << " value: " << value << std::endl;

    return true;
}

bool
RadarConfig::Load(const QDomElement& config)
{
    static Logger::ProcLog log("Load", Log());

    if (!config.hasChildNodes()) {
        LOGERROR << "no config nodes found" << std::endl;
        return false;
    }

    QString value, units;
    if (!GetEntry(config, "name", value, units)) return false;
    name_ = value.toStdString();

    if (!GetEntry(config, "gateCountMax", value, units)) return false;
    gateCountMax_ = value.toUInt();
    if (gateCountMax_ < 2) {
        LOGERROR << "invalid gateCountMax value - " << gateCountMax_ << std::endl;
        return false;
    }

    if (!GetEntry(config, "shaftEncodingMax", value, units)) return false;
    shaftEncodingMax_ = value.toUInt();
    if (shaftEncodingMax_ < 2) {
        LOGERROR << "invalid shaftEncodingMax value - " << shaftEncodingMax_ << std::endl;
        return false;
    }

    if (!GetEntry(config, "rangeMin", value, units)) return false;
    rangeMin_ = value.toDouble();

    if (!GetEntry(config, "rangeMax", value, units)) return false;
    rangeMax_ = value.toDouble();

    if (rangeMin_ >= rangeMax_ || rangeMin_ < 0.0 || rangeMax_ <= 0.0) {
        LOGERROR << "invalid range specification - " << rangeMin_ << "," << rangeMax_ << std::endl;
        return false;
    }

    if (!GetEntry(config, "rotationRate", value, units)) return false;
    rotationRate_ = value.toDouble();

    if (rotationRate_ <= 0.0) {
        LOGERROR << "invalid rotationRate value - " << rotationRate_ << std::endl;
        return false;
    }

    if (!GetEntry(config, "beamWidth", value, units)) return false;
    beamWidth_ = value.toDouble();

    if (beamWidth_ <= 0.0) {
        LOGERROR << "invalid beamWidth value - " << beamWidth_ << std::endl;
        return false;
    }

    rangeFactor_ = CalculateRangeFactor();

    if (GetEntry(config, "latitude", value, units)) {
        latitude_ = value.toDouble();
        if (units == "radians") latitude_ = Utils::radiansToDegrees(latitude_);
    }

    if (GetEntry(config, "longitude", value, units)) {
        longitude_ = value.toDouble();
        if (units == "radians") longitude_ = Utils::radiansToDegrees(longitude_);
    }

    if (GetEntry(config, "height", value, units)) {
        height_ = value.toDouble();
        if (units == "feet") height_ = Utils::feetToMeters(height_);
    }

    return true;
}
