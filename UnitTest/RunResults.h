#ifndef TESTRESULT_H		// -*- C++ -*-
#define TESTRESULT_H

#include <iosfwd>
#include <string>
#include <vector>

#include "UnitTest/TestObj.h"

namespace UnitTest 
{

/** Records statistics for all unit tests run. If any unit test throws an exception, we keep a copy of it. There
    are two types of exceptions covered: failures (represented by TestException exceptions) and errors (caused
    by anything else). The former represents a unit test that failed to pass, whereas the latter represents a
    problem running the test.
*/
class RunResults
{
public:
    
    /** Default constructor.
     */
    RunResults() throw()
    : oks_(), errs_(), fails_(), stopped_(false) {}

    /** Add a test name that passed.

        \param s the test name that passed
    */
    void addOK(const std::string& s) throw() { oks_.push_back(s); }

    /** Add an exception to the list of errors.

        \param e exception to record.
    */
    void addError(const UnitTestException& e) throw() { errs_.push_back(e); }

    /** Add an exception to the list of failures.

        \param e exception to record.
    */
    void addFailure(const UnitTestException& e) throw() { fails_.push_back(e); }

    /** \return the number of good tests so far
     */
    int numGood() const throw() { return oks_.size(); }

    /** \return the number of unit tests with errors.
     */
    int numErrors() const throw() { return errs_.size(); }

    /** \return the number of unit tests that failed to pass.
     */
    int numFailures() const throw(){ return fails_.size(); }
    
    /** \return count of unit tests that did not pass. This is the sum of numErrors and numFailures.
     */
    int numBad() const throw() { return numErrors() + numFailures(); }
    
    /** \return true if all unit tests passed (numBad() == 0)
     */
    bool passed() const throw() { return numBad() == 0; }

    /** Test if any errors or failures (! passed()).

        \return true if any errors or failures
    */
    operator bool() const throw() { return ! passed(); }

    /** \return true if asked to stop running.
     */
    bool isStopped() const throw() { return stopped_; }

    /** Stop running. Do not start any future tests.
     */
    void stopRun() throw() { stopped_ = true; }

    /** Write formatted text statistics to the given stream.

        \param os stream to write to
    */
    std::ostream& printText(std::ostream& os) const throw();

    /** Write XML statistics to the given stream.

        \param os stream to write to
    */
    std::ostream& printXML(std::ostream& os) const throw();

private:
    std::vector<std::string> oks_; ///< Names of tests that passed
    std::vector<UnitTestException> errs_; ///< Array of exceptions encountered
    std::vector<UnitTestException> fails_; ///< Array of failed tests encountered
    bool stopped_;                         ///< True if unit testing is being stopped
};

/**
   Utility function to output formatted stats to the given output stream.
*/
inline std::ostream&
operator<<(std::ostream& os, const RunResults& rr) throw() { return rr.printText(os); }

}				// namespace UnitTest

/** \file
 */

#endif
