#ifndef SIDECAR_MESSAGES_TRACK_H // -*- C++ -*-
#define SIDECAR_MESSAGES_TRACK_H

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

class Track : public Header, public IO::Printable<Track> {
    using Super = Header;

public:
    using Ref = boost::shared_ptr<Track>;

    struct Coord {
        Coord() : tuple_(){};
        Coord(double x, double y, double z = 0.0)
        {
            tuple_[0] = x;
            tuple_[1] = y;
            tuple_[2] = z;
        }
        double operator[](size_t index) const { return tuple_[index]; }
        double& operator[](size_t index) { return tuple_[index]; }
        double tuple_[3];
    };

    enum Flags { kDropping = 1, kNew, kPromoted, kNeedsPrediction, kNeedsCorrection, kPredicted, kCorrected };

    enum Type { kTentative = 1, kConfirmed };

    static Logger::Log& Log();

    static const MetaTypeInfo& GetMetaTypeInfo();

    static Ref Make(const std::string& producer);

    static Ref Make(ACE_InputCDR& cdr);

    /** Copy constructor copies the contents of report to the new Track message */
    static Ref Make(const std::string& producer, const Messages::Track::Ref& report);

    /** Destructor.
     */
    ~Track() {}

    /** Read in a Track message from an CDR input stream. Override of Header::load().

        \param cdr stream to read from

        \return stream read from
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr);

    /** Write out a Track message to a CDR output stream. Override of Header::write().

        \param cdr stream to write to

        \return stream written to
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

    /** Get and Set methods for class variables not inherited from TSPI
     */
    double getStartTime() const { return startTime_; }

    void setStartTime(double time) { startTime_ = time; }

    double getConfirmedTime() const { return confirmedTime_; }

    void setConfirmedTime(double time) { confirmedTime_ = time; }

    int getType() const { return type_; }

    void setType(int t) { type_ = t; }

    double getPredictionTime() const { return predictionTime_; }

    void setPredictionTime(double time) { predictionTime_ = time; }

    double getExtractionTime() const { return extractionTime_; }

    void setExtractionTime(double time) { extractionTime_ = time; }

    int getTrackNumber() const { return trackNum_; }

    void setTrackNumber(int n) { trackNum_ = n; }

    const Coord& getPrediction() const { return llh_prediction_; }

    void setPrediction(const Coord& llh) { llh_prediction_ = llh; }

    const Coord& getVelocity() const { return llh_velocity_; }

    void setVelocity(const Coord& llh) { llh_velocity_ = llh; }

    const Coord& getExtraction() const { return aer_extraction_; }

    void setExtraction(const Coord& aer) { aer_extraction_ = aer; }

    int getExtractionNum() const { return extractionNum_; }

    void setExtractionNum(int num) { extractionNum_ = num; }

    int getBatchNum() const { return batchNum_; }

    void setBatchNum(int n) { batchNum_ = n; }

    int getTotalInBatch() const { return totalInBatch_; }

    void setTotalInBatch(int n) { totalInBatch_ = n; }

    uint16_t getFlags() const { return flags_; }

    void setFlags(uint16_t f) { flags_ = f; }

    const Coord& getEstimate() const { return llh_; }

    double getLatitude() const { return getEstimate()[GEO_LAT]; }

    double getLongitude() const { return getEstimate()[GEO_LON]; }

    double getHeight() const { return getEstimate()[GEO_HGT]; }

    void setEstimate(const Coord& llh) { llh_ = llh; }

    double getWhen() const { return when_; }

    void setWhen(double when) { when_ = when; }

    /** Print out a textual representation of the Track message. Override of Header::print().

        \param os C++ text stream to write to

        \return stream written to
    */
    std::ostream& printData(std::ostream& os) const;

    std::ostream& printDataXML(std::ostream& os) const;

    void loadXML(XmlStreamReader& xsr);

    void dump() const;

private:
    Track(const std::string& producer);

    Track();

    ACE_InputCDR& loadV1(ACE_InputCDR& cdr);

    int batchNum_;
    int totalInBatch_;
    int trackNum_;

    Coord llh_;             ///< Last position report for track
    Coord llh_velocity_;    ///< Current velocity estimate for track
    double when_;           ///< Time of last position report
    Coord aer_extraction_;  ///< Last extraction associated with track
    double extractionTime_; ///< Time of last extraction
    int extractionNum_;     ///< Count of extractions

    Coord llh_prediction_;  ///< Most recent prediction computed for track
    double predictionTime_; ///< Time of prediction calculation

    uint16_t flags_; ///< Tentative or Confirmed */
    uint16_t type_;  ///< ???

    double startTime_;
    double confirmedTime_;

    static Header::Ref CDRLoader(ACE_InputCDR& cdr);

    static Header::Ref XMLLoader(const std::string& producer, XmlStreamReader& xsr);

    static ACE_InputCDR& LoadV1(Track* object, ACE_InputCDR& cdr);

    using LoaderRegistry = TLoaderRegistry<Track>;
    static LoaderRegistry::VersionedLoaderVector DefineLoaders();
    static LoaderRegistry loaderRegistry_;
    static MetaTypeInfo metaTypeInfo_;
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
