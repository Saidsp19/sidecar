#ifndef SIDECAR_IO_PRINTABLE_H // -*- C++ -*-
#define SIDECAR_IO_PRINTABLE_H

#include <iosfwd>
#include <string>

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** Interface definition for classes that can be written to a C++ text output stream (std::ostream) object.
    Derived classes must define a print method with the following interface:

    \code
    std::ostream& print(std::ostream& os) const
    \endcode
*/
template <typename T>
class Printable {
public:
    /** Invoke the print() method of a given object to write out its representation to an C++ output stream

        \param os stream to write to

        \param obj object to write

        \return stream written to
    */
    friend std::ostream& operator<<(std::ostream& os, const T& obj) { return obj.print(os); }
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
