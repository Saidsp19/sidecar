#include "UnitTest/UnitTest.h"

#include "PeakDetector.h"

using namespace SideCar::GUI;

struct Test : public UnitTest::TestObj {
    Test() : TestObj("PeakDetector") {}

    void test();
};

void
Test::test()
{
    PeakDetector a(3);
    assertEqual(0.0, a.getValue());
    assertTrue(a.add(1.0));
    assertEqual(1.0, a.getValue());
    assertTrue(a.add(2.0));
    assertEqual(2.0, a.getValue());
    assertTrue(a.add(3.0));
    assertEqual(3.0, a.getValue());
    assertFalse(a.add(2.0));
    assertEqual(3.0, a.getValue());
    assertFalse(a.add(1.0));
    assertEqual(3.0, a.getValue());
    assertTrue(a.add(0.0));
    assertEqual(2.0, a.getValue());
    assertTrue(a.add(0.0));
    assertEqual(1.0, a.getValue());
    assertTrue(a.add(0.0));
    assertEqual(0.0, a.getValue());
    assertTrue(a.add(3.0));
    assertEqual(3.0, a.getValue());
    assertFalse(a.add(2.0));
    assertEqual(3.0, a.getValue());
    assertFalse(a.add(1.0));
    assertEqual(3.0, a.getValue());
    assertTrue(a.add(1.0));
    assertEqual(2.0, a.getValue());
    assertTrue(a.add(1.0));
    assertEqual(1.0, a.getValue());
    a.clear(20.0);
    assertEqual(20.0, a.getValue());
    assertFalse(a.add(0.0));
    assertFalse(a.add(0.0));
    assertTrue(a.add(0.0));
    assertEqual(0.0, a.getValue());
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
