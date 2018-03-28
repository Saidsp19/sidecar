#include "QtCore/QSysInfo"

#include <cmath>
#include <map>

#include "Logger/Log.h"
#include "Utils/Utils.h"

#include "RadarConfig.h"
#include "TSPI.h"
#include "XmlStreamReader.h"

using namespace SideCar::Messages;

TSPI::LoaderRegistry::VersionedLoaderVector
TSPI::DefineLoaders()
{
    LoaderRegistry::VersionedLoaderVector loaders;
    loaders.push_back(LoaderRegistry::VersionedLoader(1, &TSPI::LoadV1));
    return loaders;
}

TSPI::LoaderRegistry TSPI::loaderRegistry_(TSPI::DefineLoaders());

MetaTypeInfo TSPI::metaTypeInfo_(MetaTypeInfo::Value::kTSPI, "TSPI", &TSPI::CDRLoader, &TSPI::XMLLoader);

Logger::Log&
TSPI::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.TSPI");
    return log_;
}

const MetaTypeInfo&
TSPI::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

const GEO_LOCATION*
TSPI::GetOrigin()
{
    static GEO_LOCATION* origin_ = 0;

    // !!! NOTE1: this is not thread-safe. Potential memory leak here. NOTE2: be careful with initialization
    // !!! since this depends on RadarConfig class attributes which we wish to be initialized and valid before
    // !!! creating the origin.
    //
    if (!origin_) {
        Logger::ProcLog log("GetOrigin", Log());
        origin_ = new GEO_LOCATION;

        // geoInitLocation() expects lat/lon in degrees, height in meters.
        //
        geoInitLocation(origin_, RadarConfig::GetSiteLatitude(), RadarConfig::GetSiteLongitude(),
                        RadarConfig::GetSiteHeight(), GEO_DATUM_DEFAULT, "Radar");
        LOGINFO << "LLH: " << origin_->lat << ' ' << origin_->lon << ' ' << origin_->hgt << std::endl;
        LOGINFO << "EFG: " << origin_->e << ' ' << origin_->f << ' ' << origin_->g << std::endl;
    }

    return origin_;
}

std::string
TSPI::GetSystemIdTag(uint16_t systemId)
{
    using Map = std::map<uint16_t, std::string>;

    // !!! NOTE: this is not thread-safe. Potential race situation here, with major badness if invoked from
    // !!! multiple threads.
    //
    static Map map_;
    if (map_.empty()) {
        map_.insert(Map::value_type(0x1201, "RFA"));
        map_.insert(Map::value_type(0x1202, "RFB"));
        map_.insert(Map::value_type(0x1001, "GTA"));
        map_.insert(Map::value_type(0x1006, "GTB"));
        map_.insert(Map::value_type(0x1002, "GTC"));
        map_.insert(Map::value_type(0x104F, "GTD"));
        map_.insert(Map::value_type(0x1050, "GTE"));
        map_.insert(Map::value_type(0x1084, "GTF"));
        map_.insert(Map::value_type(0x1085, "GTG"));
        map_.insert(Map::value_type(0x109A, "GTH"));
        map_.insert(Map::value_type(0x109B, "GTI"));
        map_.insert(Map::value_type(0x132D, "GTJ"));
        map_.insert(Map::value_type(0x103D, "MRA"));
        map_.insert(Map::value_type(0x1044, "MRB"));
        map_.insert(Map::value_type(0x1005, "MRC"));
        map_.insert(Map::value_type(0x1038, "MRD"));
        map_.insert(Map::value_type(0x1007, "MRE"));
        map_.insert(Map::value_type(0x1039, "MRF"));
        map_.insert(Map::value_type(0x1009, "MRG"));
    }

    Map::const_iterator pos = map_.find(systemId);
    if (pos != map_.end()) return pos->second;

    std::ostringstream os;
    switch (systemId & 0xF000) {
    case 0x0000: os << 'I'; break;
    case 0x1000: os << 'R'; break;
    case 0x2000: os << 'G'; break;
    default: os << 'U'; break;
    }

    os << (systemId & 0x0FFF);
    return os.str();
}

TSPI::Ref
TSPI::MakeRaw(const std::string& producer, ACE_Message_Block* raw)
{
    Logger::ProcLog log("MakeRaw", Log());
    LOGINFO << producer << ' ' << int(*raw->rd_ptr()) << ' ' << raw->length() << std::endl;
    if (*raw->rd_ptr() != 0x01 || raw->size() < 18) return Ref();
    Ref ref(new TSPI(producer, raw));
    return ref;
}

TSPI::Ref
TSPI::MakeRAE(const std::string& producer, const std::string& id, double when, double r, double a, double e)
{
    Ref ref(new TSPI(producer, id, when, InitFromRAE(r, a, e)));
    return ref;
}

TSPI::Ref
TSPI::MakeLLH(const std::string& producer, const std::string& id, double when, double lat, double lon, double hgt)
{
    Ref ref(new TSPI(producer, id, when, InitFromLLH(lat, lon, hgt)));
    return ref;
}

TSPI::Ref
TSPI::MakeXYZ(const std::string& producer, const std::string& id, double when, double x, double y, double z)
{
    Ref ref(new TSPI(producer, id, when, InitFromXYZ(x, y, z)));
    return ref;
}

TSPI::Ref
TSPI::Make(ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("Make", Log());
    LOGINFO << std::endl;
    Ref ref(new TSPI);
    ref->load(cdr);
    return ref;
}

Header::Ref
TSPI::CDRLoader(ACE_InputCDR& cdr)
{
    return Make(cdr);
}

Header::Ref
TSPI::XMLLoader(const std::string& producer, XmlStreamReader& xsr)
{
    Ref ref(new TSPI(producer));
    ref->loadXML(xsr);
    return ref;
}

double
TSPI::MakeEFGCoordinateFromRawBytes(const uint8_t* ptr)
{
    // Raw-data arrives in big-endian format. If on a little-endian machine we need to do some byte-reordering
    //
    int32_t tmp;
    if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
        // swap byte-order on little endian machine
        //
        tmp = (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
    } else { // on a big-endian machine, no nothing to bytes
        tmp = *((uint32_t*)ptr);
    }

    // TSPI values are scaled by 256 to preserve some fractional component.
    //
    return tmp / 256.0;
}

TSPI::TSPI(const std::string& producer, ACE_Message_Block* raw) :
    Header(producer, GetMetaTypeInfo()), when_(0.0), flags_(0), tag_(""), llh_(), rae_(), xyz_()
{
    Logger::ProcLog log("TSPI", Log());
    LOGINFO << std::endl;

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(raw->rd_ptr());

    uint16_t systemId = (bytes[1] << 8) | bytes[2];
    when_ = (bytes[3] << 8) | bytes[4];
    flags_ = bytes[17];
    tag_ = GetSystemIdTag(systemId);

    // Reconstruct EFG geodetic coordinates.
    //
    efg_[GEO_E] = MakeEFGCoordinateFromRawBytes(bytes + 5);
    efg_[GEO_F] = MakeEFGCoordinateFromRawBytes(bytes + 9);
    efg_[GEO_G] = MakeEFGCoordinateFromRawBytes(bytes + 13);

    if (log.showsDebug1()) dump();
}

TSPI::TSPI(const std::string& producer, const std::string& tag, double when, const InitFromRAE& rae) :
    Header(producer, GetMetaTypeInfo()), when_(when), flags_(0), tag_(tag), llh_(), rae_(), xyz_()
{
    Logger::ProcLog log("TSPI", Log());
    LOGINFO << "tag: " << tag << " when: " << when << " rng: " << rae.r_ << " az: " << rae.a_ << " el: " << rae.e_
            << std::endl;

    rae_.resize(3);
    rae_[GEO_RNG] = rae.r_;
    if (::fabs(rae.r_) < 1.0E-8) {
        rae_[GEO_AZ] = 0.0;
        rae_[GEO_EL] = 0.0;
    } else {
        rae_[GEO_AZ] = rae.a_;
        rae_[GEO_EL] = rae.e_;
    }

    geoRae2Efg(const_cast<GEO_LOCATION*>(GetOrigin()), &rae_[0], efg_);

    if (log.showsDebug1()) dump();
}

TSPI::TSPI(const std::string& producer, const std::string& tag, double when, const InitFromLLH& llh) :
    Header(producer, GetMetaTypeInfo()), when_(when), flags_(0), tag_(tag), llh_(), rae_(), xyz_()
{
    Logger::ProcLog log("TSPI", Log());
    LOGINFO << "when: " << when << " lat: " << llh.lat_ << " lon: " << llh.lon_ << " hgt: " << llh.hgt_ << std::endl;

    llh_.resize(3);
    llh_[GEO_LAT] = Utils::degreesToRadians(llh.lat_);
    llh_[GEO_LON] = Utils::degreesToRadians(llh.lon_);
    llh_[GEO_HGT] = llh.hgt_;
    geoLlh2Efg(llh_[GEO_LAT], llh_[GEO_LON], llh_[GEO_HGT], GEO_DATUM_DEFAULT, &efg_[GEO_E], &efg_[GEO_F],
               &efg_[GEO_G]);

    if (log.showsDebug1()) dump();
}

TSPI::TSPI(const std::string& producer, const std::string& tag, double when, const InitFromXYZ& xyz) :
    Header(producer, GetMetaTypeInfo()), when_(when), flags_(0), tag_(tag), llh_(), rae_(), xyz_()
{
    Logger::ProcLog log("TSPI", Log());
    LOGINFO << "when: " << when << " x: " << xyz.x_ << " y: " << xyz.y_ << " z: " << xyz.z_ << std::endl;
    xyz_.resize(3);
    xyz_[GEO_X] = xyz.x_;
    xyz_[GEO_Y] = xyz.y_;
    xyz_[GEO_Z] = xyz.z_;
    rae_.resize(3);
    geoXyz2Rae(&xyz_[0], &rae_[0]);
    if (::fabs(rae_[GEO_RNG]) < 1.0E-8) {
        rae_[GEO_AZ] = 0.0;
        rae_[GEO_EL] = 0.0;
    }

    geoRae2Efg(const_cast<GEO_LOCATION*>(GetOrigin()), &rae_[0], efg_);
    if (log.showsDebug1()) dump();
}

TSPI::TSPI() : Header(GetMetaTypeInfo()), llh_(), rae_(), xyz_()
{
    ;
}

TSPI::TSPI(const std::string& producer) : Header(producer, GetMetaTypeInfo()), llh_(), rae_(), xyz_()
{
    ;
}

void
TSPI::calculateLLH() const
{
    static Logger::ProcLog log("calculateLLH", Log());
    LOGINFO << std::endl;

    // Convert from earth-centered to longitude, latitude, height (geodetic). NOTE: geoEfg2Llh() expects lat/lon
    // in radians, height in meters.
    //
    llh_.resize(3);
    geoEfg2Llh(GEO_DATUM_DEFAULT, const_cast<double*>(efg_), &llh_[GEO_LAT], &llh_[GEO_LON], &llh_[GEO_HGT]);
    LOGINFO << "LLH: " << Utils::radiansToDegrees(llh_[GEO_LAT]) << ' ' << Utils::radiansToDegrees(llh_[GEO_LON]) << ' '
            << llh_[GEO_HGT] << std::endl;
}

const TSPI::Coord&
TSPI::getLatitudeLongitudeHeight() const
{
    if (llh_.empty()) calculateLLH();
    return llh_;
}

void
TSPI::calculateRAE() const
{
    static Logger::ProcLog log("calculateRAE", Log());
    LOGINFO << std::endl;

    if (xyz_.empty()) calculateXYZ();

    // Convert from delta meters to slant range, azimuth, elevation, with azimuth and elevation in radians.
    //
    rae_.resize(3);
    geoXyz2Rae(&xyz_[0], &rae_[0]);
    if (::fabs(rae_[GEO_RNG]) < 1.0E-8) {
        rae_[GEO_AZ] = 0.0;
        rae_[GEO_EL] = 0.0;
    }

    LOGINFO << "RAE: " << rae_[GEO_RNG] << ' ' << Utils::radiansToDegrees(rae_[GEO_AZ]) << ' '
            << Utils::radiansToDegrees(rae_[GEO_EL]) << std::endl;
}

const TSPI::Coord&
TSPI::getRangeAzimuthElevation() const
{
    if (rae_.empty()) calculateRAE();
    return rae_;
}

void
TSPI::calculateXYZ() const
{
    if (llh_.empty()) calculateLLH();

    // Create a new location for delta calculations to this point from the radar origin. NOTE: geoInitLocation()
    // expects lat/lon in degrees, height in meters.
    //
    GEO_LOCATION target;
    geoInitLocation(&target, Utils::radiansToDegrees(llh_[GEO_LAT]), Utils::radiansToDegrees(llh_[GEO_LON]),
                    llh_[GEO_HGT], GEO_DATUM_DEFAULT, "Target");

    xyz_.resize(3);
    geoEfg2XyzDiff(const_cast<GEO_LOCATION*>(GetOrigin()), &target, &xyz_[0]);
}

const TSPI::Coord&
TSPI::getXYZ() const
{
    if (xyz_.empty()) calculateXYZ();
    return xyz_;
}

ACE_InputCDR&
TSPI::load(ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("load", Log());
    LOGINFO << std::endl;
    Super::load(cdr);
    return loaderRegistry_.load(this, cdr);
}

ACE_InputCDR&
TSPI::LoadV1(TSPI* obj, ACE_InputCDR& cdr)
{
    return obj->loadV1(cdr);
}

ACE_InputCDR&
TSPI::loadV1(ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("loadV1", Log());
    LOGINFO << "BEGIN" << std::endl;

    cdr.read_double_array(efg_, 3);
    LOGDEBUG << "EFG: " << efg_[GEO_E] << ' ' << efg_[GEO_F] << ' ' << efg_[GEO_G] << std::endl;
    cdr >> when_;
    cdr >> flags_;
    cdr >> tag_;
    cdr >> attributes_;
    llh_.clear();
    rae_.clear();
    xyz_.clear();

    LOGINFO << "END" << std::endl;
    return cdr;
}

ACE_OutputCDR&
TSPI::write(ACE_OutputCDR& cdr) const
{
    Super::write(cdr);
    cdr << loaderRegistry_.getCurrentVersion();
    cdr.write_double_array(efg_, 3);
    cdr << when_;
    cdr << flags_;
    cdr << tag_;
    cdr << attributes_;
    return cdr;
}

std::ostream&
TSPI::printData(std::ostream& os) const
{
    os << "Tag: " << tag_ << " When: " << when_ << " RAE: " << getRange() << "/"
       << Utils::radiansToDegrees(getAzimuth()) << "/" << getElevation() << " Flags: " << flags_;
    return os;
}

std::ostream&
TSPI::printDataXML(std::ostream& os) const
{
    return os << "<plot tag=\"" << tag_ << "\" when=\"" << when_ << "\" range=\"" << getRange() << "\" azimuth=\""
              << Utils::radiansToDegrees(getAzimuth()) << "\" elevation=\"" << getElevation() << "\" flags=\"" << flags_
              << "\" />";
}

void
TSPI::loadXML(XmlStreamReader& xsr)
{
    Header::loadXML(xsr);
    if (!xsr.readNextEntityAndValidate("plot")) ::abort();

    when_ = xsr.getAttribute("when").toDouble();
    rae_.resize(3);
    rae_[GEO_RNG] = xsr.getAttribute("range").toDouble();
    rae_[GEO_AZ] = Utils::degreesToRadians(xsr.getAttribute("azimuth").toDouble());
    rae_[GEO_EL] = xsr.getAttribute("elevation").toDouble();
    geoRae2Efg(const_cast<GEO_LOCATION*>(GetOrigin()), &rae_[0], efg_);
    flags_ = xsr.getAttribute("flags").toShort();
}

void
TSPI::dump() const
{
    static Logger::ProcLog log("dump", Log());
    LOGDEBUG << "Tag: " << tag_ << " When: " << when_ << std::endl;
    LOGDEBUG << "EFG: " << efg_[GEO_E] << ' ' << efg_[GEO_F] << ' ' << efg_[GEO_G] << std::endl;
    LOGDEBUG << "LLH: " << Utils::radiansToDegrees(getLatitude()) << ' ' << Utils::radiansToDegrees(getLongitude())
             << ' ' << getHeight() << std::endl;
    LOGDEBUG << "RAE: " << getRange() << ' ' << Utils::radiansToDegrees(getAzimuth()) << ' '
             << Utils::radiansToDegrees(getElevation()) << std::endl;
    LOGDEBUG << "XYZ: " << getX() << ' ' << getY() << ' ' << getZ() << std::endl;
}
