#ifndef UTILS_AZIMUTHSWEEP_H // -*- C++ -*-
#define UTILS_AZIMUTHSWEEP_H

namespace Utils {

class AzimuthSweep {
public:
    AzimuthSweep(double start, double width);

    double getStart() const { return start_; }
    double getEnd() const;
    double getWidth() const { return width_; }

    /** Determine if this sweep overlaps another one. Overlapping is true if the sweeps intersect, or if one is
        completely contained in another.

        \param other the sweep to check

        \return true if so
    */
    bool overlaps(const AzimuthSweep& other) const;

    /** Determine if this sweep contains another one. Containment is true iff the given sweep's start and end
        angles reside within the range of this sweep.

        \param other the sweep to check

        \return true if so
    */
    bool contains(const AzimuthSweep& other) const;

    bool contains(double angle) const { return containsStart(angle); }

    bool operator==(const AzimuthSweep& rhs) const { return start_ == rhs.start_ && width_ == rhs.width_; }

    bool operator!=(const AzimuthSweep& rhs) const { return start_ != rhs.start_ || width_ != rhs.width_; }

private:
    /** Determine if a given sweep start value is found within our sweep range.

        \param start value to check

        \return true if value >= start && value < start + width
    */
    bool containsStart(double value) const;

    /** Determine if a given sweep end value is found within our sweep range.

        \param start value to check

        \return true if value > start && value <= start + width
    */
    bool containsEnd(double value) const;

    double start_;
    mutable double end_;
    double width_;
};

} // end namespace Utils

/** \file
 */

#endif
