#ifndef SIDECAR_UTILS_CDRSTREAMABLE_H // -*- C++ -*-
#define SIDECAR_UTILS_CDRSTREAMABLE_H

#include "ace/CDR_Stream.h"
#include <string>

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** Interface definition for classes that can be streamed into and out of CDR streams. Defines two friend
    functions (the C++ stream inserter and extractor operators) so that they will call the template argument's
    write and load methods respectively.

    The template argument _T must be a class that defines two methods with the following interfaces:

    \code
    ACE_InputCDR& load(ACE_InputCDR&);
    ACE_OutputCDR& write(ACE_OutputCDR&) const;
    \endcode

    Example:

    \code
    struct Foo : public CDRStreamable<Foo> {
      ACE_InputCDR& load(ACE_InputCDR&);
      ACE_OutputCDR& write(ACE_OutputCDR&) const;
    };

    void blah(ACE_OutputCDR& cdr, const Foo& obj)
    {
      cdr << obj;  // Uses friend operator<<() defined in CDRStreamable
    }
    \endcode
*/
template <typename T>
class CDRStreamable {
public:
    /** Invoke the load() method of a given object to read in data from an ACE CDR input stream

        \param cdr CDR input stream to read from

        \param obj object to change

        \return stream read from
    */
    friend ACE_InputCDR& operator>>(ACE_InputCDR& cdr, T& obj) { return obj.load(cdr); }

    /** Invoke the write() method of a given object to write out its representation to an ACE CDR input stream

        \param cdr CDR input stream to write to

        \param obj object to write

        \return stream written to
    */
    friend ACE_OutputCDR& operator<<(ACE_OutputCDR& cdr, const T& obj) { return obj.write(cdr); }
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
