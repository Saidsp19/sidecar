#include "AzimuthSweep.h"
#include "UnitTest/UnitTest.h"
#include "Utils.h"

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("AzimuthSweep") {}
    void test();
};

void
Test::test()
{
    Utils::AzimuthSweep filter(Utils::degreesToRadians(350.0), Utils::degreesToRadians(20.0));
    assertTrue(filter.overlaps(filter));
    assertTrue(filter.contains(filter));

    assertTrue(filter.overlaps(Utils::AzimuthSweep(Utils::degreesToRadians(350.0), Utils::degreesToRadians(0.5))));
    assertTrue(filter.contains(Utils::AzimuthSweep(Utils::degreesToRadians(350.0), Utils::degreesToRadians(0.5))));
    assertTrue(filter.overlaps(Utils::AzimuthSweep(Utils::degreesToRadians(359.0), Utils::degreesToRadians(5.0))));
    assertTrue(filter.contains(Utils::AzimuthSweep(Utils::degreesToRadians(359.0), Utils::degreesToRadians(5.0))));
    assertTrue(filter.overlaps(Utils::AzimuthSweep(Utils::degreesToRadians(359.0), Utils::degreesToRadians(30.0))));
    assertFalse(filter.contains(Utils::AzimuthSweep(Utils::degreesToRadians(359.0), Utils::degreesToRadians(30.0))));
    assertTrue(filter.overlaps(Utils::AzimuthSweep(Utils::degreesToRadians(0.0), Utils::degreesToRadians(0.5))));
    assertTrue(filter.contains(Utils::AzimuthSweep(Utils::degreesToRadians(0.0), Utils::degreesToRadians(0.5))));
    assertFalse(filter.overlaps(Utils::AzimuthSweep(Utils::degreesToRadians(10.0), Utils::degreesToRadians(0.5))));
    assertFalse(filter.contains(Utils::AzimuthSweep(Utils::degreesToRadians(10.0), Utils::degreesToRadians(0.5))));

    Utils::AzimuthSweep full(Utils::degreesToRadians(0.0), Utils::degreesToRadians(360.0));
    assertTrue(full.overlaps(full));
    assertTrue(full.contains(full));
    assertTrue(full.contains(Utils::AzimuthSweep(Utils::degreesToRadians(0.0), 0.0)));
    assertTrue(full.contains(Utils::AzimuthSweep(Utils::degreesToRadians(360.0), 0.0)));
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
    return 0;
}
