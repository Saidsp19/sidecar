#include <algorithm>
#include <functional>

#include <iostream>

#include "RunResults.h"
#include "Suite.h"

using namespace UnitTest;

/** Functor class for the Suite destructor.
 */
struct DeleteTest {
    /** Functor method. Delete a test object if we own it.

        \param p object to check
    */
    void operator()(const Suite::PairBoolTestObj& p) const
    {
        if (p.first) delete p.second;
    }
};

Suite::~Suite()
{
    std::for_each(tests_.begin(), tests_.end(), DeleteTest());
}

/** Functor class for the Suite::numTests method.
 */
struct CountTests {
    /** Constructor.
     */
    CountTests() : sum_(0) {}

    /** Functor method that accumulates number of unit tests in the suite.

        \param p entry to accumulate
    */
    void operator()(const Suite::PairBoolTestObj& p) { sum_ += p.second->numTests(); }

    /** Conversion operator that spits out the counter.
     */
    operator int() const { return sum_; }

    int sum_; ///< Unit test accumulator
};

int
Suite::numTests() const
{
    return std::for_each(tests_.begin(), tests_.end(), CountTests());
}

/** Functor class for the Suite::run method.
 */
struct RunTests {
    /** Constructor.

        \param master the Suite object being run

        \param rr collector of results from the unit tests
    */
    RunTests(Suite& master, RunResults& rr) : master_(master), rr_(rr), name_(master.testName())
    {
        name_ += "::";
    }

    /** Invoke the given unit test's run method.

        \param p unit test to run
    */
    void operator()(const Suite::PairBoolTestObj& p) const
    {
        std::string saved = p.second->testName();
        std::string tmp = name_;
        tmp += saved;
        p.second->setTestName(tmp);
        p.second->run(rr_);
        p.second->setTestName(saved);
    }

    /** Conversion operator that returns the RunResults used during unit test runs. NOTE: returning a reference
        is safe since we are just returning the value we were given in our constructor, so the lifetime of that
        object must be greater than the lifetime of us (if we were not allocated on the heap)

        \return reference to held RunResults object
    */
    operator RunResults&() const { return rr_; }

private:
    Suite& master_;
    RunResults& rr_;
    std::string name_;
};

RunResults&
Suite::run(RunResults& rr)
{
    return std::for_each(tests_.begin(), tests_.end(), RunTests(*this, rr));
}
