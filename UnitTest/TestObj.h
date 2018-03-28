#ifndef UNITTEST_TESTOBJ_H // -*- C++ -*-
#define UNITTEST_TESTOBJ_H

#include <exception> // for std::exception
#include <iostream>
#include <limits>
#include <sstream> // for std::ostringstream
#include <string>

/** \defgroup utd TestObj Function Macros
 */

/** UnitTest assertion macro that checks a C++ expression for truth. Can only be used inside a method of a
    TestObj class, since it invokes the TestObj::assertTrue() method. If the given condition evaluates to false,
    a UnitTestException is thrown which contains a string representation of the C++ expression, and the file and
    line number where the test was made.

    \param C C++ code evaluated at run-time to assert a true value

    \ingroup utd
*/
#define assertTrue(C) _assertTrue(C, #C, __FILE__, __LINE__)

/** UnitTest assertion macro that checks a C++ expression for truth. Can only be used inside a method of a
    TestObj class, since it invokes the TestObj::assertTrue() method. If the given condition evaluates to false,
    a UnitTestException is thrown which contains a string representation of the C++ expression, and the file and
    line number where the test was made.

    \param C C++ code evaluated at run-time to assert a true value

    \ingroup utd
*/
#define assertFalse(C) _assertFalse(C, #C, __FILE__, __LINE__)

/** UnitTest assertion macro that checks that two values are equal. Can only be used inside a method of a
    TestObj class, since it invokes the TestObj::assertEQ() method. Relies on the '==' operator to perform the
    equality check. If the equality check fails, a UnitTestException is thrown.

    \param E expected test value

    \param A actual test value

    \ingroup utd
*/
#define assertEqual(E, A) _assertEqual(E, A, __FILE__, __LINE__)

/** UnitTest assertion macro that checks that two values are unequal. Can only be used inside a method of a
    TestObj class, since it invokes the TestObj::_assertNotEqual() method. Relies on the '==' operator to
    perform the equality check. If the inequality check fails, a UnitTestException is thrown.

    \param E expected test value

    \param A actual test value

    \ingroup utd
*/
#define assertNotEqual(E, A) _assertNotEqual(E, A, __FILE__, __LINE__)

/** UnitTest assertion macro that checks that one value differs from another by no more than a given epsilon
    value. Can only be used inside a method of a TestObj class since it invokes the TestObj::assertEQ() method.
    If the delta between the two values is greater than the given epsilon value, a UnitTestException is thrown.

    \param E expected test value

    \param A actual test value

    \param D epsilon value

    \ingroup utd
*/
#define assertEqualEpsilon(E, A, D) _assertEqual(E, A, D, __FILE__, __LINE__)

/** UnitTest assertion macro that checks that one value differs from another by at least a given epsilon value.
    Can only be used inside a method of a TestObj class since it invokes the TestObj::assertNEQ() method. If the
    delta between the two values is less than the given epsilon value, a UnitTestException is thrown.

    \param E expected test value

    \param A actual test value

    \param D epsilon value

    \ingroup utd
*/
#define assertNotEqualEpsilon(E, A, D) _assertNotEqual(E, A, D, __FILE__, __LINE__)

namespace UnitTest {

template <typename T>
struct TypeTraits {
    static bool areEqual(const T& expected, const T& actual) { return expected == actual; }
    static bool areEqual(const T* expected, const T* actual) { return expected == actual; }

    static auto toString(const T& value) -> std::string
    {
        std::ostringstream os;
        os << value;
        return os.str();
    }

    static auto toString(const T* value) -> std::string
    {
        std::ostringstream os;
        os << value;
        return os.str();
    }
};

class RunResults;

/** Exception class thrown by a failed assertion in a unit test. Records the failure message, the file it was
    found in, the line of the file, and the name of the test that failed the assertion. For the UnitTest
    framework, there are two classifications of exceptions: those thrown by a failed assertion (assertTrue() or
    assertEQ()), and all others. The first are classified as test failures, while the latter are run-time errors
    (eg. file not found). Both classifications are represented by UnitTestException (perhaps mistakenly so).
    However, only test failures report file and line information.
*/
class UnitTestException {
public:
    /** Constructor. Remembers details of an exceptional situation.

        \param msg description of the failure or error

        \param file name of the file that raised the exception. May be NULL.

        \param line line number of the file

        \param testName name of the test that was running at the time of the exception.
    */
    UnitTestException(const char* msg, const std::string& testName) throw() :
        msg_(msg), file_(""), line_(-1), testName_(testName)
    {
    }

    UnitTestException(const std::string& msg, const std::string& testName) throw() :
        msg_(msg), file_(""), line_(-1), testName_(testName)
    {
    }

    UnitTestException(const char* msg, const char* file, int line, const std::string& testName) throw() :
        msg_(msg), file_(file), line_(line), testName_(testName)
    {
    }

    UnitTestException(const std::string& msg, const char* file, int line, const std::string& testName) throw() :
        msg_(msg), file_(file), line_(line), testName_(testName)
    {
    }

    /** Destructor.
     */
    virtual ~UnitTestException() throw() {}

    /** Return the description of the failure or error.
     */
    const std::string& err() const throw() { return msg_; }

    /** Return the name of the unit test that was running.
     */
    const std::string& testName() const throw() { return testName_; }

    /** Return the name of the file in which the exception occured. May be NULL.
     */
    const std::string& file() const throw() { return file_; }

    /** Return the line number of the file. If file() returns NULL, this is undefined.
     */
    long line() const throw() { return line_; }

    /** Write out exception text to given output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& printText(std::ostream& os) const;

    /** Write out XML representation of exception to given output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& printXML(std::ostream& os) const;

private:
    std::string msg_;      ///< Exception message
    std::string file_;     ///< File that generated the exception
    long line_;            ///< Line in file that generated exception
    std::string testName_; ///< Test that was running
};

/** Interface and baseline functionality for all unit test objects. Can be used as a simple unit test by
    subclassing and overriding TestObj::test() method. Here's a simple check to make sure that 1 is really one.

    \code
    struct Foo : public TestObj
    {
    Foo() : one_(1) {}
    void test() { assertEqual(1, one_); }
    int one_;
    };

    int
    main()
    {
    return Foo().mainRun();  // run the above test
    }
    \endcode

    Please note that \c assertEqual above is not a member of \c TestObj or of \c Foo. Rather, it is a C
    preprocessor function macro found in TestObj.h. Although this is a gross violation of standard coding
    practices, these C preprocessor function macros allow a unit test to record the file and line number where
    the test was if it failed. They do so by using the values of the __FILE__ and __LINE__ C preprocessor value
    macros as arguments to the TestObj methods they invoke (the alternative would be to have the developer type
    in __FILE__ and __LINE__ for each invocation of _assertEqual() or its kin)

    See \ref utd for a documentation on these function macros.
*/
class TestObj {
public:
    /** Default contructor.
     */
    TestObj() throw() : name_("") {}

    /** Contructor. Takes the given name as its own. Note that test names may be changed by a container object.

        \param name name of this unit test object.
    */
    TestObj(const std::string& name) throw() : name_(name) {}

    /** Destructor.
     */
    virtual ~TestObj() {}

    /** Get the number of tests perfomed by this object. Subclasses that represent multiple test would return
        something different.

        \return the number of unit tests this object represents.
    */
    virtual int numTests() const throw() { return 1; }

    /** Execute the unit test(s) this object represents. This is basically a wrapper for the protected
        TestObj::test method that traps an records any exceptions that occur during the unit test.

        \param rr collector of run statistics

        \return value of input parameter
    */
    virtual RunResults& run(RunResults& rr) throw();

    /** Utility method useful in main() to run unit tests. Prints out the unit test results to the given stream
        (if not NULL), and then returns EXIT_SUCCESS or EXIT_FAILURE depending on whether all the test pass.

        \param os stream to write the unit test results to.

        \return EXIT_SUCCESS if all tests pass; otherwise EXIT_FAILURE.
    */
    virtual int mainRun(std::ostream* os = &std::cerr) throw();

    /** Set the test name to the given value. Derived classes and container clases use this to manipulate the
        test name.

        \param name new name to assume
    */
    void setTestName(const std::string& name) throw() { name_ = name; }

    /** Get the name of this test object.

        \return test name
    */
    const std::string& testName() const throw() { return name_; }

    /** Assertion for a true value. Raises a UnitTestException exception if the given value is not true. Passes
        other arguments to exception constructor.

        \param value value to check

        \param expr string representation of value

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    void _assertTrue(bool value, const char* expr, const char* file, int line) const throw(UnitTestException)
    {
        if (!value) throw UnitTestException(expr, file, line, testName());
    }

    /** Assertion for a good std::istream value. Raises a UnitTestException exception if the given value is not
        true. Passes other arguments to exception constructor.

        \param value value to check

        \param expr string representation of value

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    void _assertTrue(std::istream& value, const char* expr, const char* file, int line) const throw(UnitTestException)
    {
        if (!value.good()) throw UnitTestException(expr, file, line, testName());
    }

    /** Assertion for a false value. Raises a UnitTestException exception if the given value is true. Passes
        other arguments to exception constructor.

        \param value value to check

        \param expr string representation of value

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    void _assertFalse(bool value, const char* expr, const char* file, int line) const throw(UnitTestException)
    {
        if (value) throw UnitTestException(expr, file, line, testName());
    }

    /** Assertion for a good std::istream value. Raises a UnitTestException exception if the given value is not
        true. Passes other arguments to exception constructor.

        \param value value to check

        \param expr string representation of value

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    void _assertFalse(std::istream& value, const char* expr, const char* file, int line) const throw(UnitTestException)
    {
        if (value.good()) throw UnitTestException(expr, file, line, testName());
    }

    /** Assertion that two values of some type T are equal. Raises a UnitTestException exception if they are
        different. Attempts to compare the values to each other. If they are different, then compare their
        string representations.

        \param expected value that was expected in the unit test

        \param actual value that was obtained from the unit test

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    template <typename T>
    void _assertEqual(const T& expected, const T& actual, const char* file, int line) const throw(UnitTestException)
    {
        if (!TypeTraits<T>::areEqual(expected, actual)) {
            _assertEqual(TypeTraits<T>::toString(expected), TypeTraits<T>::toString(actual), file, line);
        }
    }

    /** Assertion that two values of some type pointer are equal. Raises a UnitTestException exception if they
        are different. Attempts to compare the values to each other. If they are different, then compare their
        string representations.

        \param expected value that was expected in the unit test

        \param actual value that was obtained from the unit test

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    template <typename T>
    void _assertEqual(const T* expected, const T* actual, const char* file, int line) const throw(UnitTestException)
    {
        if (!TypeTraits<T>::areEqual(expected, actual)) {
            _assertEqual(TypeTraits<T>::toString(expected), TypeTraits<T>::toString(actual), file, line);
        }
    }

    /** Assertion that two strings are equal. Raises a UnitTestException exception if they are different.

        \param expected value that was expected in the unit test

        \param actual value that was obtained from the unit test

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    void _assertEqual(std::string expected, std::string actual, const char* file, int line) const
        throw(UnitTestException)
    {
        if (expected != actual) throwError("expected", expected, actual, file, line);
    }

    void _assertEqual(double expected, double actual, const char* file, int line) const throw(UnitTestException)
    {
        _assertEqual(expected, actual, std::numeric_limits<double>::epsilon(), file, line);
    }

    /** Assertion that two values are equal within some tolerance factor. Raises a UnitTestException exception
        if two values differ more than the given epsilon value. Passes other arguments to exception constructor.

        \param expected value that was expected in the unit test

        \param actual value that was obtained from the unit test

        \param epsilon tolerance factor used for testing

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    void _assertEqual(double expected, double actual, double epsilon, const char* file, int line) const
        throw(UnitTestException);

    /** Assertion that two values are NOT equal. Raises a UnitTestException exception if two values are exactly
        the same. Equality is checked using the '!=' operator, and then by comparing the textual representations
        of the given values (using the toString() template method). Passes other arguments to exception
        constructor.

        \param expected value that was expected in the unit test

        \param actual value that was obtained from the unit test

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    template <typename T>
    void _assertNotEqual(const T& expected, const T& actual, const char* file, int line) const throw(UnitTestException)
    {
        if (TypeTraits<T>::areEqual(expected, actual)) {
            throwError("did not expect", TypeTraits<T>::toString(expected), TypeTraits<T>::toString(actual), file,
                       line);
        }
    }

    template <typename T>
    void _assertNotEqual(const T* expected, const T* actual, const char* file, int line) const throw(UnitTestException)
    {
        if (TypeTraits<T>::areEqual(expected, actual)) {
            throwError("did not expect", TypeTraits<T>::toString(expected), TypeTraits<T>::toString(actual), file,
                       line);
        }
    }

    /** Assertion that two strings are NOT equal. Raises a UnitTestException exception if they are the same.

        \param expected value that was expected in the unit test

        \param actual value that was obtained from the unit test

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    void _assertNotEqual(std::string expected, std::string actual, const char* file, int line) const
        throw(UnitTestException)
    {
        if (expected == actual) throwError("did not expected", expected, actual, file, line);
    }

    void _assertNotEqual(double expected, double actual, const char* file, int line) const throw(UnitTestException)
    {
        _assertNotEqual(expected, actual, std::numeric_limits<double>::epsilon(), file, line);
    }

    /** Assertion that two values are NOT equal within some tolerance factor. Raises a UnitTestException
        exception if two values do not differ more than the given epsilon value. Passes other arguments to
        exception constructor.

        \param expected value that was expected in the unit test

        \param actual value that was obtained from the unit test

        \param epsilon tolerance factor used for testing

        \param file name of the file in which the assertion was made

        \param line line number of the file where the assertion was made
    */
    void _assertNotEqual(double expected, double actual, double epsilon, const char* file, int line) const
        throw(UnitTestException);

protected:
    /** Method to perform the unit test. Derived classes must override to do something useful. Default behavior
        is to assert false, which always raises an exception.
    */
    virtual void test() { assertTrue(false); }

private:
    /** Construct a failed unit test exception and throw.

        \param tag prefix to exception text

        \param expected expected value as a std::string

        \param actual actual value from unit test as a std::string

        \param file name of file the test resides in

        \param line line of the file that the test is found on
    */
    void throwError(const char* tag, const std::string& expected, const std::string& actual, const char* file,
                    int line) const throw(UnitTestException);

    /** Construct a failed unit test exception and throw.

        \param tag prefix to exception text

        \param expected expected value

        \param actual actual value from unit test

        \param delta unsigned difference between the two values

        \param epsilon largest delta acceptable for a valid test

        \param file name of file the test resides in

        \param line line of the file that the test is found on
    */
    void throwError(const char* tag, double expected, double actual, double delta, double epsilon, const char* file,
                    int line) const throw(UnitTestException);

    std::string name_; ///< Name of the test.
};                     // class TestObj

} // end namespace UnitTest

/** \file
 */

#endif
