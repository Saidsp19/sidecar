#ifndef UTILS_UTILS_H // -*- C++ -*-
#define UTILS_UTILS_H

#include <cmath> // for std::floor
#include <fstream>
#include <inttypes.h> // for uint32_t and siblings
#include <iostream>
#include <string>

#include "Utils/Exception.h" // for Utils::Exception

namespace Utils {

/** Find elevation from range and altitude, taking the curvature of the earth into consideration.

    \param range slant range in meters

    \param altitude altitude in meters

    \param refLatitudeRadians latitude of the vector's origin in radians

    \return elevation in radians
*/
double el_from_range_and_alt(double range, double altitude, double refLatitudeRadians);

/** Calculate the greatest common divisor for two integral numbers.

    \param x first number

    \param y second number

    \return GCD value
*/
template <typename T>
constexpr T
GCD(T x, T y)
{
    return (x == y) ? x : ((x > y) ? GCD(x - y, y) : GCD(x, y - x));
}

/** Calculate the least common multiple for two integral numbers

    \param x first number

    \param y second number

    \return LCM value
*/
template <typename T>
constexpr T
LCM(T x, T y)
{
    return x / GCD(x, y) * y; // GCD should always return >= 1
}

extern double const kCircleRadians;

/** Create a full path from the given file and path values. Any previous path is stripped from the file before
    it is used. Assumes, but does not check, that path ends in a '/'.

    \param path value for path to file

    \param file value for the file name

    \return path + file
*/
extern std::string makePath(const std::string& path, const std::string& file);

inline double
radiansToDegrees(double value)
{
    return value * 180.0 / M_PI;
}

inline double
degreesToRadians(double value)
{
    return value * M_PI / 180.0;
}

double normalizeRadians(double value);

double normalizeDegrees(double value);

uint32_t leastPowerOf2(uint32_t value);

inline double
feetToMeters(double value)
{
    return value * 0.3048;
}

inline double
metersToFeet(double value)
{
    return value * 3.2808399;
}

/** Simple class that prohibits derived classes from begin copied.
 */
class Uncopyable {
protected:
    Uncopyable() {}

private:
    Uncopyable(const Uncopyable&);
    Uncopyable& operator=(const Uncopyable&);
};

/** Meta-program that calculates 2**M from within the compiler.
 */
template <unsigned int M>
struct PowerOf2 : public PowerOf2<M - 1> {
    enum { kValue = 2 * PowerOf2<M - 1>::kValue };
    static int GetValue() { return kValue; }
};

/** Meta-program that calculates 2**0 from within the compiler. Used to terminate the recursive inheritance
    above.
*/
template <>
struct PowerOf2<0> {
    enum { kValue = 1 };
    static int GetValue() { return kValue; }
};

/** Adapter for an iterator that performs some type of conversion for each value obtained from a forward
    iterator. The first template argument identifies the type of the iterator used to fetch values. The second
    is a functor that must define an operator() method which does the conversion from one type to another.
*/
template <typename _Iterator, typename _Converter>
class ConverterAdapter : public std::iterator<std::forward_iterator_tag, typename _Converter::argument_type> {
public:
    /** Constructor

        \param __x

        \param __y
    */
    ConverterAdapter(const _Iterator& __x, const _Converter& __y) : it(__x), co(__y) {}

    ConverterAdapter<_Iterator, _Converter> operator=(const typename _Converter::argument_type& __x)
    {
        it = co(__x);
        return *this;
    }

    ConverterAdapter<_Iterator, _Converter> operator*() { return *this; }
    ConverterAdapter<_Iterator, _Converter> operator++()
    {
        ++it;
        return *this;
    }
    ConverterAdapter<_Iterator, _Converter> operator++(int) { return ConverterAdapter(it++, co); }

protected:
    _Iterator it;
    _Converter co;
};

/** Factory function that returns a new ConverterAdapter object.

    \param __fn iterator to use

    \param __co functor to use

    \return
*/
template <typename _Operation, typename _Converter>
inline ConverterAdapter<_Operation, _Converter>
converterAdapter(const _Operation& __fn, const _Converter& __co)
{
    return ConverterAdapter<_Operation, _Converter>(__fn, __co);
}

/** Iterator adapter that attempts to convert std::string values into other types.
 */
template <class _Iterator>
class StringConverterAdapter : public std::iterator<std::forward_iterator_tag, std::string> {
public:
    StringConverterAdapter(const _Iterator& __x) : it(__x) {}

    StringConverterAdapter<_Iterator> operator=(const std::string& __x)
    {
        typename _Iterator::container_type::value_type __y;
        __x >> __y;
        it = __y;
        return *this;
    }

    StringConverterAdapter<_Iterator> operator*() { return *this; }
    StringConverterAdapter<_Iterator> operator++()
    {
        ++it;
        return *this;
    }
    StringConverterAdapter<_Iterator> operator++(int) { return StringConverterAdapter(it++); }

protected:
    _Iterator it;
};

template <class _Iterator>
inline StringConverterAdapter<_Iterator>
stringConverterAdapter(const _Iterator& __fn)
{
    return StringConverterAdapter<_Iterator>(__fn);
}

} // namespace Utils

namespace std {

/**
   Simple template method used to convert from a string value into some other type using the >> operator. Relies
   on the std::istringstream class to do the real work.

   \param s std::string to convert

   \param v container to hold the converted value

   \return state of std::istringstream after conversion
*/
template <class T>
inline bool
operator>>(const std::string& s, T& v) throw()
{
    return (std::istringstream(s) >> v).good();
}

} // namespace std

/** \file
 */

#endif
