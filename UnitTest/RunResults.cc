#include <algorithm>
#include <iostream>

#include "RunResults.h"

using namespace UnitTest;

inline std::ostream&
operator<<(std::ostream& os, const UnitTestException& e)
{
    return e.printText(os);
}

/** Functor class used to print out an error or failure record.
 */
struct PrintText {
    /** Constructor.

        \param os stream to write to.
    */
    PrintText(std::ostream& os) throw() : os_(os){};

    /** Print exception

        \param e exception to print
    */
    void operator()(const UnitTestException& e) const throw() { e.printText(os_) << '\n'; }

    /** Print test name

        \param s test name to print
    */
    void operator()(const std::string& s) const throw() { os_ << s << '\n'; }

private:
    std::ostream& os_;
};

/** Utility function that prints out a set of exceptions -- either test failures or run-time errors.
 */
static void
printSet(std::ostream& os, const std::vector<UnitTestException>& v, const char* tag)
{
    int count = v.size();
    os << "--- " << count << ' ' << tag;
    if (count != 1) os << 's';
    os << ':' << std::endl;
    std::for_each(v.begin(), v.end(), PrintText(os));
}

std::ostream&
RunResults::printText(std::ostream& os) const throw()
{
    if (passed()) {
        os << "+++ PASSED - " << numGood() << " passed\n";
    } else {
        os << "*** FAILED - " << numGood() << " passed; " << numFailures() << " failed; " << numErrors() << " error";
        if (numErrors() != 1) os << 's';
        os << std::endl;
    }

    os << "--- " << oks_.size() << " good:\n";
    std::for_each(oks_.begin(), oks_.end(), PrintText(os));

    if (errs_.size()) printSet(os, errs_, "error");
    if (fails_.size()) printSet(os, fails_, "failure");

    return os;
}

struct PrintXML {
    /** Constructor.

        \param os stream to write to.

        \param type type of record being written ("Assertion" or "Exception")

        \param counter ID of the test that is being printed
    */
    PrintXML(std::ostream& os, const std::string& type, int counter) throw() :
        os_(os), type_(type), counter_(counter){};

    /** Print exception.

        \param e the assertion or exception to record
    */
    void operator()(const UnitTestException& e) throw()
    {
        os_ << "  <FailedTest id=\"" << ++counter_ << "\">\n"
            << "   <FailureType>" << type_ << "</FailureType>\n";
        e.printXML(os_);
        os_ << "  </FailedTest>\n";
    }

    /** Print test name

        \param s the test name to record
    */
    void operator()(const std::string& s) throw()
    {
        os_ << "  <Test id=\"" << ++counter_ << "\">\n"
            << "   <Name>" << s << "</Name>\n"
            << "  </Test>\n";
    }

    /** Obtain the current entity counter value.

        \return number of entries written so far
    */
    operator int() const { return counter_; }

private:
    std::ostream& os_;
    std::string type_;
    int counter_;
};

std::ostream&
RunResults::printXML(std::ostream& os) const throw()
{
    os << "<?xml version=\"1.0\" encoding='utf-8' ?>\n"
       << "<TestRun>\n"
       << " <FailedTests>\n";

    int counter = 0;
    counter = std::for_each(fails_.begin(), fails_.end(), PrintXML(os, "Assertion", counter));
    counter = std::for_each(errs_.begin(), errs_.end(), PrintXML(os, "Error", counter));
    os << " </FailedTests>\n"
       << " <SuccessfulTests>\n";

    counter = std::for_each(oks_.begin(), oks_.end(), PrintXML(os, "", counter));
    os << " </SuccessfulTests>\n"
       << " <Statistics>\n"
       << "  <Tests>" << counter << "</Tests>\n"
       << "  <FailuresTotal>" << (fails_.size() + errs_.size()) << "</FailuresTotal>\n"
       << "  <Errors>" << errs_.size() << "</Errors>\n"
       << "  <Failures>" << fails_.size() << "</Failures>\n"
       << " </Statistics>\n"
       << "</TestRun>\n";

    return os;
}
