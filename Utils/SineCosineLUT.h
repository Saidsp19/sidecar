#ifndef UTILS_SINECOSINELUT_H // -*- C++ -*-
#define UTILS_SINECOSINELUT_H

#include <utility> // for std::pair
#include <vector>

#include "Utils/Exception.h"

namespace Logger {
class Log;
}

namespace Utils {

/** Simple table lookup class for sine/cosine values. Note that this does not support interpolation, only
    indexed lookup, where the indices range from [0,size) and linearly map to [0,2*PI) radians. One source of
    indices that behave like this is a radar shaft encoder that emits a unique integer value depending on where
    the antenna is pointing, with a resolution of N values.

    The lookup table contains sine and cosine values calculated for N / 4 indices. The lookup() method uses
    various trig identities to calculate the remaining values for the rest of the indices. Because of this space
    reduction, the size of the lookup table must be a multiple of 4.
*/
class SineCosineLUT {
public:
    /** Obtain the Log device to use for objects of this class.

        \return Loggger::Log reference
    */
    static Logger::Log& Log();

    /** Exception thrown if the requested table size is not a multiple of 4.
     */
    struct InvalidSize : public Utils::Exception {
    };

    /** Factory method that creates a new lookup table. Checks the \p size argument to see that it is a multiple
        of 4.

        \param size number of integral values in a circle (2 * PI)

        \return new SineCosineLUT object or NULL if invalid size
    */
    static SineCosineLUT* Make(size_t size);

    /** Constructor. Creates and populates the lookup table. Throws an exception if the given size is not a
        multiple of 4.

        \param size number of integral values in a circle (2 * PI)
    */
    SineCosineLUT(size_t size);

    /** Obtain the sine and cosine values for a given lookup index

        \param index integral value in [0,size) to fetch

        \param sine reference to storage for sine value

        \param cosine reference to storage for cosine value
    */
    void lookup(size_t index, double& sine, double& cosine) const;

    size_t size() const { return size_; }

private:
    /** Internal structure that holds the calculate sine/cosine values.
     */
    struct SineCosine {
        SineCosine() {}
        SineCosine(double s, double c) : sine(s), cosine(c) {}
        double sine;
        double cosine;
    };

    std::vector<SineCosine> values_; ///< Lookup table for 1 quadrant of indices
    size_t size_;                    ///< Number of indices in a circle
};

} // end namespace Utils

#endif
