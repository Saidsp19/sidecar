#ifndef SIDECAR_MESSAGES_TSPI_H // -*- C++ -*-
#define SIDECAR_MESSAGES_TSPI_H

#include <cmath>
#include <vector>

#include "boost/shared_ptr.hpp"

#include "GeoStars/geoStars.h"
#include "IO/CDRStreamable.h"
#include "IO/Printable.h"
#include "Messages/Attributes.h"
#include "Messages/Header.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Messages {

class TSPI : public Header, public IO::Printable<TSPI> {
public:
    using Super = Header;
    using Ref = boost::shared_ptr<TSPI>;

    using Coord = std::vector<double>;

    enum Flags { kDropping = (1 << 0) };

    static Logger::Log& Log();

    /** Obtain the message type information for RawVideo objects.

        \return MetaTypeInfo reference
    */
    static const MetaTypeInfo& GetMetaTypeInfo();

    static const GEO_LOCATION* GetOrigin();

    static std::string GetSystemIdTag(uint16_t systemId);

    /** Utility that converts 4 raw bytes into a double value, taking into account endianess of the host, where
        big-endian is taken as-is and little-endian is byte-swapped.

        \param ptr address of 4 bytes to use

        \return floating-point value
    */
    static double MakeEFGCoordinateFromRawBytes(const uint8_t* ptr);

    /** Class factory that creates new reference-counted TSPI message objects.

        \param producer name of the entity that is creating the new object

        \param raw data block containing the raw TSPI message

        \return reference to new TSPI object
    */
    static Ref MakeRaw(const std::string& producer, ACE_Message_Block* raw);

    /** Class factory that creates new reference-counted TSPI message objects from range, azimuth, and elevation
        values.

        \param producer name of the entity that created the message

        \param tag name for the TSPI plot

        \param when time of the plot

        \param range distance (slant-range) to target in meters

        \param azimuth angle from 0 north to target in radians

        \param elevation angle from horizontal to target in radians

        \param latitude degrees latitude of the target

        \param longitude degrees longitude of the target

        \param height vertical distance to target in meters

        \return reference to new TSPI object
    */
    static Ref MakeRAE(const std::string& producer, const std::string& tag, double when, double range, double azimuth,
                       double elevation);

    /** Class factory that creates new reference-counted TSPI message objects from latitude, longitude, and
        height values.

        \param producer name of the entity that created the message

        \param tag name for the TSPI plot

        \param when time of the plot

        \param latitude degrees latitude of the target

        \param longitude degrees longitude of the target

        \param height vertical distance to target in meters

        \return reference to new TSPI object
    */
    static Ref MakeLLH(const std::string& producer, const std::string& tag, double when, double latitude,
                       double longitude, double height);

    /** Class factory that creates new reference-counted TSPI message objects using offsets from the radar
        position (given by RadarConfig)

        \param producer name of the entity that created the message

        \param tag name for the TSPI plot

        \param when time of the plot

        \param x north offset from radar in meters

        \param y east offset from radar in meters

        \param z up offset from radar in meters

        \return reference to new TSPI object
    */
    static Ref MakeXYZ(const std::string& producer, const std::string& tag, double when, double x, double y, double z);

    /** Class factory that creates new reference-counted TSPI message objects using data from an input CDR
        stream.

        \param cdr input CDR stream to read from

        \return reference to new TSPI object
    */
    static Ref Make(ACE_InputCDR& cdr);

    /** Destructor.
     */
    ~TSPI() {}

    /** Read in a TSPI message from an CDR input stream. Override of Header::load().

        \param cdr stream to read from

        \return stream read from
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr);

    /** Write out a TSPI message to a CDR output stream. Override of Header::write().

        \param cdr stream to write to

        \return stream written to
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

    /** Print out a textual representation of the TSPI message. Override of Header::print().

        \param os C++ text stream to write to

        \return stream written to
    */
    std::ostream& printData(std::ostream& os) const;

    /** Write out a TSPI message to a C++ output stream in XML format.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& printDataXML(std::ostream& os) const;

    /** Obtain the position in earth-centered coordinates

        \return earth-centered cartesian coordinates in meters
    */
    const double* getEarthCentered() const { return efg_; }

    /** Obtain the E component of the earth-centered position

        \return E value
    */
    double getE() const { return getEarthCentered()[GEO_E]; }

    /** Obtain the F component of the earth-centered position

        \return F value
    */
    double getF() const { return getEarthCentered()[GEO_F]; }

    /** Obtain the G component of the earth-centered position

        \return G value
    */
    double getG() const { return getEarthCentered()[GEO_G]; }

    /** Obtain the position in latitude, longitude, height

        \return latitude (radians), longitude (radians), and height (meters).
    */
    const Coord& getLatitudeLongitudeHeight() const;

    /** Obtain the latitude component

        \return value in radians
    */
    double getLatitude() const { return getLatitudeLongitudeHeight()[GEO_LAT]; }

    /** Obtain the longitude component

        \return value in radians
    */
    double getLongitude() const { return getLatitudeLongitudeHeight()[GEO_LON]; }

    /** Obtain the height component

        \return value in meters
    */
    double getHeight() const { return getLatitudeLongitudeHeight()[GEO_HGT]; }

    /** Obtain the position in range, azimuth, elevation from the radar origin.

        \return range (meters), azimuth (radians), elevation (radians)
    */
    const Coord& getRangeAzimuthElevation() const;

    /** Obtain the range component

        \return value in meters
    */
    double getRange() const { return getRangeAzimuthElevation()[GEO_RNG]; }

    /** Obtain the azimuth component

        \return value in radians
    */
    double getAzimuth() const { return getRangeAzimuthElevation()[GEO_AZ]; }

    /** Obtain the elevation component

        \return value in radians
    */
    double getElevation() const { return getRangeAzimuthElevation()[GEO_EL]; }

    /** Obtain the 3-D offsets from the radar origin.

        \return x (meters), y (meters), z (meters) offsets from radar
    */
    const Coord& getXYZ() const;

    /** Obtain the X component

        \return value in meters
    */
    double getX() const { return getXYZ()[GEO_X]; }

    /** Obtain the Y component

        \return value in meters
    */
    double getY() const { return getXYZ()[GEO_Y]; }

    /** Obtain the Z component

        \return value in meters
    */
    double getZ() const { return getXYZ()[GEO_Z]; }

    /** Obtain the timestamp for the position report

        \return time value
    */
    double getWhen() const { return when_; }

    /** Obtain the name assigned to the report

        \return name
    */
    const std::string& getTag() const { return tag_; }

    /** Determine if the report is dropping

        \return true if so
    */
    bool isDropping() const { return flags_ & kDropping; }

    /** Set the dropping indicator
     */
    void setDropping() { flags_ |= kDropping; }

    /** Write the current position values to the debug logger
     */
    void dump() const;

    /** Add an attribute to the position report

        \param name the name of the value

        \param value the value to use
    */
    void addAttribute(const std::string& name, const XmlRpc::XmlRpcValue& value) { attributes_.add(name, value); }

    /** Obtain the Attributes object for this report.

        \return Attributes object
    */
    const Attributes& getAttributes() const { return attributes_; }

    /** Set internal state from an XML message.

        \param xsr input XML stream to read from
    */
    void loadXML(XmlStreamReader& xsr);

private:
    struct InitFromRAE {
        InitFromRAE(double r, double a, double e) : r_(r), a_(a), e_(e) {}
        double r_;
        double a_;
        double e_;
    };

    struct InitFromLLH {
        InitFromLLH(double lat, double lon, double hgt) : lat_(lat), lon_(lon), hgt_(hgt) {}
        double lat_;
        double lon_;
        double hgt_;
    };

    struct InitFromXYZ {
        InitFromXYZ(double x, double y, double z) : x_(x), y_(y), z_(z) {}
        double x_;
        double y_;
        double z_;
    };

    /** Constructor for new TSPI message.

        \param producer name of the entity that created the message
    */
    TSPI(const std::string& producer);

    /** Constructor for new TSPI message.

        \param producer name of the entity that created the message

        \param raw data block containing raw VME data.
    */
    TSPI(const std::string& producer, ACE_Message_Block* raw);

    TSPI(const std::string& producer, const std::string& tag, double when, const InitFromRAE& init);

    TSPI(const std::string& producer, const std::string& tag, double when, const InitFromLLH& init);

    TSPI(const std::string& producer, const std::string& tag, double when, const InitFromXYZ& init);

    /** Constructor for RawVideo messages that will be filled in with data from a CDR stream.
     */
    TSPI();

    void calculateLLH() const;
    void calculateRAE() const;
    void calculateXYZ() const;

    ACE_InputCDR& loadV1(ACE_InputCDR& cdr);

    double when_;
    double efg_[3];
    uint16_t flags_;
    std::string tag_;
    mutable Coord llh_;
    mutable Coord rae_;
    mutable Coord xyz_;
    Attributes attributes_;

    static Header::Ref CDRLoader(ACE_InputCDR& cdr);

    static Header::Ref XMLLoader(const std::string& producer, XmlStreamReader& xsr);

    static ACE_InputCDR& LoadV1(TSPI* object, ACE_InputCDR& cdr);

    using LoaderRegistry = TLoaderRegistry<TSPI>;
    static LoaderRegistry::VersionedLoaderVector DefineLoaders();
    static LoaderRegistry loaderRegistry_;

    static MetaTypeInfo metaTypeInfo_;
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
