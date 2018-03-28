#ifndef SIDECAR_UTILS_BEAMWIDTHFILTER_H // -*- C++ -*-
#define SIDECAR_UTILS_BEAMWIDTHFILTER_H

#include <cmath>
#include <vector>

#include "IO/Printable.h"

namespace Utils {

/** Simple averaging filter for azimuth deltas. Maintains a running average of N (2000 default) beam widths in
    order to obtain a slowly varying beam width value. Also provides the average number of radials one would
    expect to find in a complete rotation of the radar. The expectation when using this filter is that the
    angular velocity of the radar remains fairly constant, even when subjected to wind gusts.
*/
class BeamWidthFilter : public SideCar::IO::Printable<BeamWidthFilter> {
public:
    /** Constructor.

        \param windowSize number of width samples to keep around in the running average calculation. The larget
        the number, the more damping occurs.
    */
    BeamWidthFilter(size_t windowSize = 2000) :
        widths_(windowSize, 0.0), nextSlot_(0), runningSum_(0.0), filterValue_(0.0), filled_(false)
    {
    }

    /** Add a new value to the running average.

        \param beamWidth new sample value

        \return running average
    */
    double add(double beamWidth);

    /** Obtain the current damped value.

        \return damped beam width value
    */
    double getFilterValue() const { return filterValue_; }

    /** Obtain the number of radials in a circle that corresponds to the current filter value.

        \return radial count
    */
    size_t getRadialCount() const { return static_cast<size_t>(::ceil((2.0 * M_PI) / filterValue_)); }

    /** Dump the contents of the filter to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& print(std::ostream& os) const;

private:
    std::vector<double> widths_;
    size_t nextSlot_;
    double runningSum_;
    double filterValue_;
    bool filled_;
};

} // end namespace Utils

/** \file
 */

#endif
