#ifndef UNITTEST_PROCSUITE_H // -*- C++ -*-
#define UNITTEST_PROCSUITE_H

#include <iostream>
#include <string>
#include <utility> // for std::pair
#include <vector>

#include "UnitTest/TestObj.h"
#include "Utils/Utils.h"

namespace UnitTest {

/** Template class that represents a collection of unit test methods within one class. Extends the TestObj
    interface to register one or more methods of a class that perform the actual unit tests. Executing
    ProcSuite::run() calls them all in the order in which they were registered. Here's a quick example of how to
    set up a ProcSuite:

    @code
    struct Foobar : public UnitTest::TestObj
    {
    void testFoobar() { assertEqual(2, 2); }
    static void Install(UnitTest::ProcSuite<Foobar>& ps)
    {
    // Register the method testFoobar with the given ProcSuite object.
    ps.add("testFoobar", &Foobar::testFoobar);
    }
    };

    int
    main()
    {
    UnitTest::ProcSuite<Foobar> ps("ProcSuite");
    Foobar::Install(ps);
    return ps.mainRun();
    }
    @endcode
*/
template <typename T>
class ProcSuite : public TestObj, public Utils::Uncopyable {
public:
    /** Definition of type that represents a method pointer for class T.
     */
    using Proc = void (T::*)();

    /** Definition of type stored in std::vector collection. First element of std::pair is the name of the unit
        test, and the second is the pointer to the class method to call to perform the test.
    */
    using NamedProc = std::pair<std::string, Proc>;

    /** Constructor with existing T object that will be used to call the all of the registered methods. Adds a
        '.' to the name as a separator -- in the ProcSuite::run method, test names are prepended with this name
        so that any logged failures have a full inheritance path name.

        \param obj object to use for registered method calls

        \param name name of this test collection
    */
    ProcSuite(T* obj, const std::string& name) : TestObj(name), obj_(obj), own_(0), procs_() {}

    /** Constructor which creates a new T object to be used to call the all of the registered methods. Adds a
        '.' to the name as a separator -- in the ProcSuite::run method, test names are prepended with this name
        so that any logged failures have a full inheritance path name.

        \param name name of this test collection
    */
    ProcSuite(const std::string& name) : TestObj(name), obj_(new T), own_(1), procs_() {}

    /** Destructor. Cleans up any previously-allocated test object.
     */
    ~ProcSuite()
    {
        if (own_ && obj_) delete obj_;
        obj_ = 0;
    }

    /** \return the number of unit tests this object represents. Unlike Suite::numTests, this is simply the
        value of std::vector::size().
    */
    virtual int numTests() const { return procs_.size(); }

    /** Execute the unit tests this object represents. Loops thru all of the registered unit test methods, and
        invokes each one, recording their pass/fail value. See also ProcSuite::test() below.

        \param rr RunResults object to record the test results

        \return RunResults object that was used
    */
    virtual RunResults& run(RunResults& rr)
    {
        // Append the test name to our own for error messages.
        //
        std::string saved = testName();
        for (iter_ = procs_.begin(); iter_ != procs_.end(); ++iter_) {
            std::string tmp = saved;
            tmp += "::";
            tmp += iter_->first;
            setTestName(tmp);

            // Call parent class method, which will invoke our overridden test() below using the current
            // iterator value.
            //
            TestObj::run(rr);

            // Restore our original name.
            //
            setTestName(saved);
        }
        return rr;
    }

    /** Add the given class T method to our set of unit test methods. Associate the name with the method for
        logging purposes.

        \param name name for the unit test

        \param p pointer to the class method to call to run a unit test
    */
    void add(const char* name, Proc p)
    {
        std::clog << "add: name: " << name << " proc: " << p << std::endl;
        procs_.push_back(NamedProc(name, p));
        iter_ = procs_.begin();
        std::clog << "add: iter->first: " << iter_->first << " iter->second: " << iter_->second << std::endl;
    }

protected:
    /** Method to perform the unit test. Here, we call the registered object method to perform the unit test.
        Any exceptions encountered here will be caught in the TestObj::run() method.
    */
    virtual void test()
    {
        // Override the proxy object's name with the full test name.
        //
        std::clog << "test: " << testName() << " obj: " << obj_ << " count: " << procs_.size() << std::endl;
        obj_->setTestName(testName());
        std::clog << "iter->first: " << iter_->first << " iter->second: " << iter_->second << std::endl;
        (obj_->*(iter_->second))();
    }

private:
    /** Object used for all test method calls
     */
    T* obj_;
    bool own_;

    /** Set of registered test methods to call.
     */
    std::vector<NamedProc> procs_;

    /** Iterator that points to the next test to execute. This is a state value so that the ProcSuite::run() and
        ProcSuite::test() methods can communicate the current test to run.
    */
    typename std::vector<NamedProc>::iterator iter_;
};

} // namespace UnitTest

/** \file
 */

#endif
