#ifndef UTILS_ATOMICOP_H // -*- C++ -*-
#define UTILS_ATOMICOP_H

#include "boost/detail/atomic_count.hpp"
#include "boost/operators.hpp"

namespace Utils {

/** Class that increments and decrements an integral value in an atomic way, thus safe for use in threaded
    processing.
*/
class AtomicOp : public boost::totally_ordered<AtomicOp> {
public:
    using ValueType = long;

    /** Constructor.

        \param value initial value for the counter
    */
    AtomicOp(ValueType value = 0) : value_(value) {}

    /** Atomically increment the held counter value.
     */
    void operator++() { ++value_; }

    /** Atomically decrement the held counter value.

        \return true if new counter value is zero
    */
    ValueType operator--() { return --value_; }

    /** Obtain the current value

        \return current value
    */
    ValueType getValue() const { return value_; }

    /** Determine if held value is less than the value of another AtomicOp

        \param rhs value to compare

        \return true if so
    */
    bool operator<(const AtomicOp& rhs) const { return getValue() < rhs.getValue(); }

    /** Determine if held value is the same as the value of another AtomicOp

        \param rhs value to compare

        \return true if so
    */
    bool operator==(const AtomicOp& rhs) const { return getValue() == rhs.getValue(); }

private:
    boost::detail::atomic_count value_; ///< Current counter value
};

} // end namespace Utils

/** \file
 */

#endif
