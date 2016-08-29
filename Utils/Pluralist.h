#ifndef UTILS_PLURALIST_H	// -*- C++ -*-
#define UTILS_PLURALIST_H

#include <iosfwd>
#include <string>

#include "IO/Printable.h"

namespace Utils {

/** Utility template class used by the plural template function to temporarily save the data to be written to an
    ostream.
*/
template <typename T>
class Pluralist : public SideCar::IO::Printable< Pluralist<T> >
{
public:
    /** Constructor. Simply save the arguments.

        \param value numeric value to write out

        \param tag units description to follow the numeric value
    */
    Pluralist(const T& value, const std::string& tag) : value_(value), tag_(tag) {}

    /** Write the saved data to the given output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& print(std::ostream& os) const { return os << value_ << ' ' << tag_; }

private:
    T value_;			//> Value checked for plurality
    std::string tag_;		//> Tag to write out following value
};

/** Utility (silly?) template function that prints out a value followed by a text tag that is plural when
    necessary. To be used as an output stream inserter, like @code std::cout << Utils::plural(10, "chipmunk") <<
    std::endl; @endcode

    \param value numeric value to write out

    \param sing text representing a singular tag

    \param plur text representing a plural tag. If none is given, one is created by appending an `s' to the sing
    value.

    \return new Pluralist<T> output stream inserter
*/
template <typename T> inline Utils::Pluralist<T>
plural(const T& value, const std::string& sing, std::string plur = "")
{
    if (plur.size() == 0) plur = sing + "s";
    return Utils::Pluralist<T>(value, value == 1 ? sing : plur);
}

} // namespace Utils

/** \file
 */

#endif
