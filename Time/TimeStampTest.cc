#include <iostream>
#include <iomanip>
#include <sstream>
#include <typeinfo>

#include "boost/assign/std/vector.hpp" // for operator+=()

#include "UnitTest/UnitTest.h"

#include "TimeStamp.h"
#include "TimeStampRange.h"
#include "TimeStampRangeSet.h"

using namespace SideCar::Time;
using namespace boost::assign;	// bring 'operator+=()' into scope

struct Test : public UnitTest::ProcSuite<Test>
{
    Test() : UnitTest::ProcSuite<Test>(this, "TimeStampTests")
	{
	    add("TimeStamp", &Test::testTimeStamp);
	    add("TimeSpecifications", &Test::testTimeSpecifications);
	    add("TimeStampRange", &Test::testTimeStampRange);
	    add("TimeStampRangeSet", &Test::testTimeStampRangeSet);
	}

    void testTimeStamp();
    void testTimeSpecifications();
    void testTimeStampRange();
    void testTimeStampRangeSet();
};

void
Test::testTimeStamp()
{
    TimeStamp start = TimeStamp::Now();

    assertEqual(0, TimeStamp::Min().getSeconds());
    assertEqual(0, TimeStamp::Min().getMicro());

    assertEqual(std::numeric_limits<int>::max(),
                TimeStamp::Max().getSeconds());
    assertEqual(1E6 - 1, TimeStamp::Max().getMicro());

    TimeStamp t0;
    assertEqual(TimeStamp(0.0), t0);

    TimeStamp t1(1.5);
    assertEqual(1, t1.getSeconds());
    assertEqual(TimeStamp::kMicrosPerSecond / 2, t1.getMicro());
    assertEqualEpsilon(1.5, t1.asDouble(), 0.00001);
    t1 = 1.111;

    TimeStamp t2(2.222);
    assertEqualEpsilon(2.222, t2.asDouble(), 0.00001);

    TimeStamp t3(3.333);
    assertEqualEpsilon(3.333, t3.asDouble(), 0.00001);
    assertEqual(3, t3.getSecondsRounded());

    TimeStamp t4(t3);
    assertTrue(t1 < t2);
    assertTrue(t2 > t1);
    assertTrue(t1 != t2);

    assertFalse(t2 < t1);
    assertFalse(t1 > t2);
    assertFalse(t1 == t2);

    assertTrue(t3 == t4);
    assertFalse(t3 != t4);
    assertFalse(t3 < t4);
    assertFalse(t3 > t4);
    assertTrue(t3 <= t4);
    assertTrue(t3 >= t4);

    assertEqualEpsilon((t2 - t1).asDouble(), t1.asDouble(), 0.0001);
    assertEqualEpsilon((t2 + t1).asDouble(), t3.asDouble(), 0.0001);

    TimeStamp end = TimeStamp::Now();
    assertTrue(start < end);

    TimeStamp t5(10, TimeStamp::kMicrosPerSecond - 1);
    assertEqual(11, t5.getSecondsRounded());
    t5 += TimeStamp(0, (TimeStamp::kMicrosPerSecond >> 1) * -1);
    assertEqual(10, t5.getSecondsRounded());
    t5 += TimeStamp(0, 2);
    assertEqual(11, t5.getSecondsRounded());
}

void
Test::testTimeSpecifications()
{
    try {
	// Empty specification
	//
	TimeStamp::ParseSpecification("", TimeStamp::Now());
	assertTrue(0);
    }
    catch (const TimeStamp::InvalidSpecification&) {
	;
    }
    catch (const Utils::Exception&) {
	;
    }

    try {
	// Bogus specification
	//
	TimeStamp::ParseSpecification("hi", TimeStamp::Now());
	assertTrue(0);
    }
    catch (const TimeStamp::InvalidSpecification&) {
	;
    }
    catch (const Utils::Exception&) {
	;
    }

    // Relative seconds (integer)
    //
    TimeStamp now = TimeStamp::Now();
    TimeStamp z = TimeStamp::ParseSpecification("+1", now);
    assertEqual(now + TimeStamp(1 , 0), z);

    // Relative seconds (floating-point)
    //
    z = TimeStamp::ParseSpecification("+1.5", now);
    assertEqual(now + TimeStamp(1 , TimeStamp::kMicrosPerSecond / 2), z);

    // Relative seconds in MM:SS
    //
    z = TimeStamp::ParseSpecification("+10:02", now);
    assertEqual(now + TimeStamp(602 , 0), z);

    // Relative time seconds MM:SS.SSS
    //
    z = TimeStamp::ParseSpecification("+1:02.5", now);
    assertEqual(now + TimeStamp(62 , TimeStamp::kMicrosPerSecond / 2), z);

    // Absolute time. Obtain SSS:UUU representation of known value, then parse it and make sure we get back the
    // original TimeStamp value.
    //
    {
	std::ostringstream os;
	os << now;
	z = TimeStamp::ParseSpecification(os.str(), now);
	assertEqual(now, z);
    }

    {
	std::ostringstream os;
	TimeStamp future(now);
	future += 123;
	os << future;
	z = TimeStamp::ParseSpecification(os.str(), now);
	assertEqual(future, z);
    }
}

void
Test::testTimeStampRange()
{
    // Test for illegal formats
    //
    try {
	TimeStampRange::Make(""); // empty string
	assertTrue(0);
    }
    catch (const std::runtime_error&) {
	;
    }

    try {
	TimeStampRange::Make("barf"); // bad starting value
	assertTrue(0);
    }
    catch (const std::runtime_error&) {
	;
    }

    try {
	TimeStampRange::Make("123.233&"); // expecting '-'
	assertTrue(0);
    }
    catch (const std::runtime_error&) {
	;
    }

    try {
	TimeStampRange::Make("123.233-barf"); // bad ending value
	assertTrue(0);
    }
    catch (const std::runtime_error&) {
	;
    }

    try {
	TimeStampRange::Make("456-123"); // start > end
	assertTrue(0);
    }
    catch (...) {
	assertTrue(1);
    }

    // Now test for valid formats
    //
    TimeStampRange a(TimeStampRange::Make("123"));// end time only
    assertEqual(TimeStamp::Min(), a.getStart());
    assertEqual(TimeStamp(123.0) , a.getEnd());

    TimeStampRange b(TimeStampRange::Make("123-")); // start time only
    assertEqual(TimeStamp(123.0) , b.getStart());
    assertEqual(TimeStamp::Max(), b.getEnd());

    TimeStampRange c(TimeStampRange::Make("123-456")); // both start and end
    assertEqual(TimeStamp(123.0), c.getStart());
    assertEqual(TimeStamp(456.0), c.getEnd());

    assertEqual(true, a.contains(TimeStamp::Min()));
    assertEqual(true, a.contains(123.0));
    assertEqual(false, a.contains(123.1));

    assertEqual(false, b.contains(TimeStamp::Min()));
    assertEqual(true, b.contains(123.0));
    assertEqual(true, b.contains(TimeStamp::Max()));

    assertEqual(false, c.contains(TimeStamp::Min()));
    assertEqual(true, c.contains(123.0));
    assertEqual(true, c.contains(456.0));
    assertEqual(false, c.contains(TimeStamp::Max()));

    assertTrue(a < b);
    assertTrue(a < c);
    assertTrue(c < b);
}

void
Test::testTimeStampRangeSet()
{
    std::vector<std::string> specs;
    specs += "100", "200-300", "400-500", "600-";
    assertEqual(4U, specs.size());

    TimeStampRangeSet tsrs(specs);
    assertFalse(tsrs.empty());
    assertEqual(4U, tsrs.size());
    
    assertTrue(tsrs.contains(TimeStamp::Min()));
    assertTrue(tsrs.contains(200.0));
    assertTrue(tsrs.contains(300.0));
    assertTrue(tsrs.contains(400.0));
    assertTrue(tsrs.contains(500.0));
    assertTrue(tsrs.contains(600.0));
    assertTrue(tsrs.contains(TimeStamp::Max()));

    assertFalse(tsrs.contains(100.1));
    assertFalse(tsrs.contains(199.9));
    assertFalse(tsrs.contains(300.1));
    assertFalse(tsrs.contains(399.9));
    assertFalse(tsrs.contains(500.1));
    assertFalse(tsrs.contains(599.9));
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
