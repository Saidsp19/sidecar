#ifndef UNITTEST_SUITE_H	// -*- C++ -*-
#define UNITTEST_SUITE_H

#include <string>
#include <utility>		// for std::pair
#include <vector>

#include "UnitTest/TestObj.h"
#include "Utils/Utils.h"

namespace UnitTest {

class RunResults;

/** Represents a collection of TestObj objects. Extends TestObj interface to register one or more unit tests.
    Executing Suite::run() exercises all of them, in the order that they were registered. Because Suite only
    relies on the TestObj interface, nesting of Suite objects (and other container types) is possible. Here's a
    simple example:

    \code
    struct Foobar : public UnitTest::TestObj
    {
    void test() { assertEqual(2, 2); }
    };

    int
    main()
    {
    UnitTest::Suite s("Suite");
    s.add(new Foobar);
    return s.mainRun();
    }
    \endcode
*/
class Suite : public TestObj, Utils::Uncopyable
{
public:

    /** Definition of type stored in std::vector collection. First element of std::pair holds a flag indicating
        if the second object is to be deleted upon destruction of the Suite object. The second element of the
        pair is a pointer to a TestObj object.
    */
    using PairBoolTestObj = std::pair<bool,TestObj*>;
    
    /** Constructor.

        \param name name of the test suite
    */
    Suite(const std::string& name) throw() : TestObj(name), tests_() {}

    /** Destructor. Deletes all registered tests which we own.
     */
    virtual ~Suite() throw();

    /** Returns the number of unit tests this object represents. The returned value represents the sum of
        numTests calls for each registered unit test. Note that this value may not be the same as what
        std::vector::size() would return, because one or more registered objects may itself be a Suite object.

        \return number of registered tests
    */
    virtual int numTests() const throw();

    /** Execute the unit tests this object represents. Loops thru all of the registered unit test objects, and
        calls their run method.

        \param rr collector of run statistics

        \return value of input parameter
    */
    virtual RunResults& run(RunResults& rr) throw();

    /** Add the given test object to our collection. Note that there is no way to remove tests once added.

        \param obj test object to add

        \param own if true, assume ownership of the object and delete it when the Suite instance is destroyed.
    */
    void add(TestObj* obj, bool own = true) throw() { tests_.push_back(PairBoolTestObj(own, obj)); }

    /** Add a reference to a test object, probably something allocated on the stack, and not the heap. Does not
        take ownership of the object.

        \param obj test object to add

        @warning Do not use a temporary (eg. add(TestObj());) since the object
        will be destroyed after the add method returns, and then your test case
        will run on a destructed object, which is bad.
    */
    void add(TestObj& obj) throw() { tests_.push_back(PairBoolTestObj(false, &obj)); }

private:

    /** Set of test cases to run, represented by TestObj objects.
     */
    std::vector<PairBoolTestObj> tests_;
};

} // end namespace UnitTest

/** \file
 */

#endif
