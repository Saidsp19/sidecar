#ifndef UTILS_WRAPPER_H		// -*- C++ -*-
#define UTILS_WRAPPER_H

#include <iosfwd>
#include <string>

#include "IO/Printable.h"

namespace Utils {

/** Utility class that will print out a string of text with lines no longer than a certain maximum, breaking the
    line at spaces.
*/
class Wrapper : public SideCar::IO::Printable<Wrapper>
{
public:

    /** Constructor.

	\param data text to print out

	\param width maximum line length to generate

	\param cursor initial value to use for the character count

	\param prefix text to write at the start of new lines
    */
    Wrapper(const std::string& data, int width, int cursor, const std::string& prefix);

    /** Output the text to an output stream, wrapping when necessary.

	\param os stream to write to

	\return stream written to 
    */
    std::ostream& print(std::ostream& os) const;

private:
    std::string data_;		///< Text to wrap
    std::string prefix_;	///< Prefix written before continuation lines
    int width_;			///< Max width of a line
    int cursor_;		///< Current position in a line being filled
};				// class Wrapper

/** Utility that creates a new Wrapper instance

    \param data text to print out

    \param width maximum line length to generate

    \param cursor initial value to use for the character count

    \param prefix text to write at the start of new lines

    \return new Wrapper instance
*/
inline Wrapper
wrap(const std::string& data, int width = 80, int cursor = 0, const std::string& prefix = "... ")
{ return Utils::Wrapper(data, width, cursor, prefix); }

/** Utility that creates a new Wrapper instance

    \param data text to print out

    \param prefix text to write at the start of new lines

    \return new Wrapper instance
*/
inline Wrapper
wrap(const std::string& data, const std::string& prefix) { return Utils::Wrapper(data, 80, 0, prefix); }

} // namespace Utils

/** \file
 */

#endif
