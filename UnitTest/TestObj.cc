#include <sys/types.h>
#include <sys/wait.h>

#include <cmath>		// for std::abs
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "TestObj.h"
#include "RunResults.h"

using namespace UnitTest;

static std::string currentTest_ = "";

std::ostream&
UnitTestException::printText(std::ostream& os) const
{
    if (file_.size()) os << file_ << ':' << line_ << ':';
    return os << testName_ << ':' << msg_;
}

std::ostream&
UnitTestException::printXML(std::ostream& os) const
{
    os << "   <Name>" << testName_ << "</Name>\n";
    if (file_.size())
	os << "   <Location>\n"
	   << "    <File>" << file_ << "</File>\n"
	   << "    <Line>" << line_ << "</Line>\n"
	   << "   </Location>\n";
    os << "   <Message>" << msg_ << "</Message>\n";
    return os;
}

/** Utility class that installs an unexpected exception handler during the instance's lifetime.
 */
struct UH
{
    /** Constructor. Installs new handler and remembers the old one

        \param f new handler to install
    */
    UH() : prev(std::set_unexpected(&UH::Unexpected)) {}
    
    /** Destructor. Restores the previous unexpected exception handler.
     */
    ~UH() { std::set_unexpected(prev); }

private:

    std::unexpected_handler prev;

    static void Unexpected()
	{
	    std::cerr << "*** unexpected error *** - " << currentTest_ << std::endl;
	    throw std::bad_exception();
	}
};

RunResults&
TestObj::run(RunResults& rr) throw()
{
    // Run test as long as unit testing is not stopped.
    //
    if (! rr.isStopped()) {
	std::cerr << "... " << name_ << std::endl;
	try {
	    currentTest_ = name_;
	    test();
	    rr.addOK(testName());
	}
	catch(const UnitTestException& e) {
	    std::cerr << "*** caught UnitTestException" << std::endl;
	    rr.addFailure(e);
	}
	catch (const std::exception& e) {
	    std::cerr << "*** caught std::exception" << std::endl;
	    rr.addError(UnitTestException(e.what(), currentTest_));
	}
	catch (...) {
	    std::cerr << "*** caught unknown error" << std::endl;
	    rr.addError(UnitTestException("unknown error", currentTest_));
	}
    }
    return rr;
}

void
TestObj::_assertEqual(double expected, double actual, double epsilon, const char* file, int line) const
    throw(UnitTestException)
{
    double delta = std::abs(expected - actual);
    if (delta > epsilon) throwError("expected", expected, actual, delta, epsilon, file, line);
}

void
TestObj::_assertNotEqual(double expected, double actual, double epsilon, const char* file, int line) const
    throw(UnitTestException)
{
    double delta = std::abs(expected - actual);
    if (delta <= epsilon) throwError("did not expect", expected, actual, delta, epsilon, file, line);
}

void
TestObj::throwError(const char* tag, const std::string& expected, const std::string& actual, const char* file,
                    int line) const throw(UnitTestException)
{
    std::ostringstream os("");
    os << tag << " [" << expected << "], got [" << actual << "]";
    std::string s(os.str());
    std::cerr << "*** throwing UnitTestException - " << s << std::endl;
    throw UnitTestException(s, file, line, testName());
}

void
TestObj::throwError(const char* tag, double expected, double actual, double delta, double epsilon,
                    const char* file, int line) const throw(UnitTestException)
{
    std::ostringstream os("");
    os.precision(10);
    os << tag << " [" << expected << "], got [" << actual << "] - delta: " << delta << " epsilon: [" << epsilon
       << ']';
    std::string s(os.str());
    std::cerr << "*** throwing UnitTestException - " << s << std::endl;
    throw UnitTestException(s, file, line, testName());
}

int
TestObj::mainRun(std::ostream* os) throw()
{
    char* fileName = ::getenv("UNIT_TEST_DIR");
    if (fileName && *fileName) {
	std::string path(fileName);
	path += '/';
	path += testName();
	path += ".xml";
	os = new std::ofstream(path.c_str());
    }

    UnitTest::RunResults rr;
    run(rr);

    if (os) {
	char* useXML = ::getenv("UNIT_TEST_XML");
	if (useXML && *useXML > '0')
	    rr.printXML(*os);
	else
	    rr.printText(*os);
    }

    if (fileName && *fileName) {
	static_cast<std::ofstream*>(os)->close();
	delete os;
    }
    
    return rr.passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}
