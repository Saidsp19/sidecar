#ifndef SIDECAR_MESSAGES_RADARCONFIG_H // -*- C++ -*-
#define SIDECAR_MESSAGES_RADARCONFIG_H

#include <cmath>
#include <inttypes.h>
#include <string>

class QDomElement;
class QString;

namespace Logger {
class Log;
}
namespace SideCar {

namespace Configuration {
class RadarConfigFileWatcher;
}

namespace Messages {


/** Run-time configuration parameters that describe the radar. Since there is only one radar configuration in
    use at a time, this class supports no instances; all methods and attributes belong to the class.

    The RadarConfig 'instance' relies on data found in an XML configuration file. The internal Initializer class
    looks in the following locations (where SIDECAR_CONFIG and SIDECAR are process environment variables):

    - ${SIDECAR_CONFIG}
    - ${SIDECAR}/data/configuration.xml
    - /opt/sidecar/data/configuration.xml

    The above locations are ordered by priority -- a configuration file found via ${SIDECAR_CONFIG} will
    override one the could be found via ${SIDECAR}/data/configuration.xml.

    NOTE: do not inline the accessor methods below. We need to execute code in RadarConfig.cc in order to make
    sure that the static attributes initialize properly; C++ does not guarantee initialization of class (static)
    attributes until code containing the attribubtes first executes.

    NOTE: the rationale for having RadarConfig done this way versus a singleton is lost to history. Perhaps
    backwards capability with some other system?
*/
class RadarConfig {
public:
    /** Log device for RadarConfig messages

        \return Log device
    */
    static Logger::Log& Log();

    static bool IsLoaded();
    
    /** Load in a new radar configuration from an XML DOM node

        \param config the DOM node containing the configuration

        \return true if successful
    */
    static bool Load(const QDomElement& config);

    /** Load in new radar configuration from raw values.

        \param name name of the configuration

        \param gateCountMax max number of gate samples in a message

        \param shaftEncodingMax max shaft encoding value

        \param rotationRate expected rotation rate of the radar in RPMs

        \param rangeMin range value of first gate sample in kilometers

        \param rangeMax range value of last gate sample in kilometers

        \param beamWidth width of radar beam for one PRI
    */
    static void Load(const std::string& name, uint32_t gateCountMax, uint32_t shaftEncodingMax, double rotationRate,
                     double rangeMin, double rangeMax, double beamWidth);

    /** Obtain the current radar configuration name

        \return config name
    */
    static const std::string& GetName();

    /** Obtain the current max gate count value

        \return max gate count
    */
    static uint32_t GetGateCountMax();

    /** Obtain the current max shaft encoding value

        \return max shaft encoding value
    */
    static uint32_t GetShaftEncodingMax();

    /** Obtain the current rotation rate value.

        \return rotation rate in RPMs
    */
    static double GetRotationRate();

    /** Obtain the current min range value, the range value associated with the first sample.

        \return min range value
    */
    static double GetRangeMin_deprecated();

    /** Obtain the current max range value, the range value associated with the gateCountMax_ - 1 sample.

        \return max range value
    */
    static double GetRangeMax();

    /** Obtain the multiplier used to convert from gate sample index values to a range value in kilometers.

        \return range factor
    */
    static double GetRangeFactor_deprecated();

    /** Obtain the current beam width value

        \return beam width
    */
    static double GetBeamWidth();

    /** Obtain the number radials in one revolution of the radar. This is an upper bound.

        \return
    */
    static uint32_t GetRadialCount() { return uint32_t(::ceil(2.0 * M_PI / GetBeamWidth())); }

    /** Obtain the range value for a gate sample index.

        \param gateIndex value to convert

        \return range in kilometers
    */
    static double GetRangeAt_deprecated(uint32_t gateIndex)
    {
        return gateIndex * GetRangeFactor_deprecated() + GetRangeMin_deprecated();
    }

    /** Obtain an azimuth reading in radians from a shaft encoding value

        \param shaftEncoding value to convert

        \return azimuth in radians
    */
    static double GetAzimuth(uint32_t shaftEncoding)
    {
        return M_PI * 2.0 * shaftEncoding / (GetShaftEncodingMax() + 1);
    }

    /** Obtain the duration in seconds of one rotation of the radar.

        \return duration in seconds
    */
    static double GetRotationDuration() { return 60.0 / GetRotationRate(); }

    /** Obtain the latitude (north/south) of the site location.

        \return degrees latitude
    */
    static double GetSiteLatitude();

    /** Obtain the longitude (east/west) of the site location.

        \return degrees longitude
    */
    static double GetSiteLongitude();

    /** Obtain the height of the site location.

        \return height in meters
    */
    static double GetSiteHeight();

private:
    /** Prohibit instances of RadarConfig class.
     */
    RadarConfig();

    static bool GetEntry(const QDomElement& config, const QString& name, QString& value, QString& units);

    /** Calculate the range factor value, which is dependent on the rangeMin_, rangeMax_, and gateCountMax_
        values.

        \return range factor
    */
    static double CalculateRangeFactor();

    static std::string name_;
    static uint32_t gateCountMax_;
    static uint32_t shaftEncodingMax_;
    static double rotationRate_;
    static double rangeMin_;
    static double rangeMax_;
    static double rangeFactor_;
    static double beamWidth_;
    static double latitude_;  ///< Radar site longitude (degrees)
    static double longitude_; ///< Radar site latitude (degrees)
    static double height_;    ///< Radar site height (meters)
};

} // namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
