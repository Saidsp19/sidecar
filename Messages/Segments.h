#ifndef SIDECAR_MESSAGES_SEGMENTS_H
#define SIDECAR_MESSAGES_SEGMENTS_H

#include "IO/CDRStreamable.h"
#include "IO/Printable.h"
#include "Messages/Header.h"
#include "MetaTypeInfo.h"

#include "boost/shared_ptr.hpp"

#include <list>

namespace SideCar {
namespace Messages {

/**
   Segments are a form of run-length encoding for video.

   Two uses for them:
   - as a run-length encode of a single PRI
   - as a 2-dimensional bitmask for an extraction
*/
class Segment {
public:
    Segment(size_t azimuth = 0, size_t start = 0, size_t stop = 0) : azimuth(azimuth), start(start), stop(stop) {}

    bool operator==(const Segment& rhs) const
    {
        return azimuth == rhs.azimuth && start == rhs.start && stop == rhs.stop;
    }

    /// azimuth index
    size_t azimuth;
    /// minimum range index
    size_t start;
    /// maximum range index
    size_t stop;
};

/**
   Store a bunch of segments and maintain info about their distribution.
*/
class SegmentList {
private:
    using VideoT = int16_t;

public:
    SegmentList() : numSegments(0), span(0), cellCount(0), peakPower(std::numeric_limits<VideoT>::min()) {}

    // takes all segments from the other object -- assumes they share a common depth
    void merge(SegmentList& other)
    {
        segments.splice(segments.end(), other.segments);
        numSegments += other.numSegments;
        other.numSegments = 0;

        if (other.span > span) span = other.span;

        cellCount += other.cellCount;
        other.cellCount = 0;
        // boundingBox+=other.boundingBox;
    }

    void merge(const Segment& s)
    {
        segments.push_back(s);
        numSegments++;
        cellCount += s.stop - s.start + 1;
        // boundingBox.addX(s.azimuth()); // problems with 2pi wrap
        // boundingBox.widenY(s);
    }

    void pop(const SegmentList& other)
    {
        std::list<Segment>::const_iterator index;
        std::list<Segment>::const_iterator stop = other.segments.end();
        for (index = other.segments.begin(); index != stop; index++) { pop(*index); }
    }

    void pop(const Segment& s)
    {
        std::list<Segment>::iterator index;
        std::list<Segment>::const_iterator stop = segments.end();
        for (index = segments.begin(); index != stop; /* inc in loop */) {
            if (s == *index) {
                index = segments.erase(index);
            } else {
                index++;
            }
        }
    }

    inline size_t PRISpan() const { return span; }
    inline void setPRISpan(size_t x) { span = x; }

    // what's the best way to expose the list?
    const std::list<Segment>& data() { return segments; }

    /// Return the number of cells in this list
    size_t getCellCount() const { return cellCount; }

    ACE_InputCDR& load(ACE_InputCDR& cdr);
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;
    std::ostream& print(std::ostream& os) const;

private:
    // Primary data (stored to disk)
    //
    size_t numSegments; // so we don't have to traverse the list to find out
    std::list<Segment> segments;
    size_t span; // number of azimuth angles spanned by this list
    // Derived data (not stored to disk?)
    //
    size_t cellCount;

public: // for the moment
    VideoT peakPower;
    size_t peakRange, peakAzimuth;
    double totalPower, centroidRange, centroidAzimuth;
    int distMinRange, distMaxRange, distMinAzimuth, distMaxAzimuth; // distances to the cutoff points
};

class SegmentMessage : public Header, public IO::Printable<SegmentMessage> {
public:
    SegmentMessage() :
        Header("FromFile", GetMetaTypeInfo(), Header::Ref()), list(new SegmentList), rangeMin_(), rangeFactor_()
    {
    }

    SegmentMessage(const std::string& producer, const Header::Ref& basis, double rangeMin, double rangeFactor) :
        Header(producer, GetMetaTypeInfo(), basis), list(new SegmentList()), rangeMin_(rangeMin),
        rangeFactor_(rangeFactor)
    {
    }

    // Takes control of the SegmentList (i.e. deletes on destruction)
    SegmentMessage(const std::string& producer, const Header::Ref& basis, SegmentList* l, double rangeMin,
                   double rangeFactor) :
        Header(producer, GetMetaTypeInfo(), basis),
        list(l), rangeMin_(rangeMin), rangeFactor_(rangeFactor)
    {
    }

    // For MessageManager::getNative()
    using Ref = boost::shared_ptr<SegmentMessage>;
    using DataRef = boost::shared_ptr<SegmentList>;

    // Header interface
    //
    ACE_InputCDR& load(ACE_InputCDR& cdr);
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

    double getRangeMin() const { return rangeMin_; }

    double getRangeFactor() const { return rangeFactor_; }

    double getRangeAt(size_t gateIndex) const { return rangeFactor_ * gateIndex + rangeMin_; }

    // Printable interface
    //
    std::ostream& printData(std::ostream& os) const;

    // For MetaTypeInfo
    //
public:
    static const MetaTypeInfo& GetMetaTypeInfo();
    static Header::Ref Loader(ACE_InputCDR& cdr);

private:
    static MetaTypeInfo metaTypeInfo;

    // Custom functionality
    //
public:
    DataRef& data() { return list; }

private:
    DataRef list;
    double rangeMin_;
    double rangeFactor_;
};

} // namespace Messages
} // namespace SideCar

/** \file
 */

#endif
