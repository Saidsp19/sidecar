#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"
#include "Utils/Utils.h"

#include "Vector.h"
#include "UnitVector.h"

using namespace SideCar::Geometry;

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("UnitVector") {}
    void test();
};

void
Test::test()
{
    UnitVector a;
    assertEqual(0.0, a.getDirection());
    assertEqual(0.0, a.getElevation());
    assertEqual(1.0, a.getX());
    assertEqual(0.0, a.getY());
    assertEqual(0.0, a.getZ());

    a = UnitVector(Utils::degreesToRadians(45), 0.0);
    assertEqual(Utils::degreesToRadians(45), a.getDirection());
    assertEqual(0.0, a.getElevation());
    assertEqualEpsilon(0.707107, a.getX(), 0.00001);
    assertEqualEpsilon(0.707107, a.getY(), 0.00001);
    assertEqualEpsilon(0.0, a.getZ(), 0.00001);

    a = UnitVector(Utils::degreesToRadians(135), 0.0);
    assertEqual(Utils::degreesToRadians(135), a.getDirection());
    assertEqual(0.0, a.getElevation());
    assertEqualEpsilon(0.707107, a.getX(), 0.00001);
    assertEqualEpsilon(-0.707107, a.getY(), 0.00001);
    assertEqualEpsilon(0.0, a.getZ(), 0.00001);

    a = UnitVector(Utils::degreesToRadians(180), 0.0);
    assertEqual(Utils::degreesToRadians(180), a.getDirection());
    assertEqual(0.0, a.getElevation());
    assertEqualEpsilon(0.0, a.getX(), 0.00001);
    assertEqualEpsilon(-1.0, a.getY(), 0.00001);
    assertEqualEpsilon(0.0, a.getZ(), 0.00001);

    a = UnitVector(Utils::degreesToRadians(225), 0.0);
    assertEqual(Utils::degreesToRadians(225), a.getDirection());
    assertEqual(0.0, a.getElevation());
    assertEqualEpsilon(-0.707107, a.getX(), 0.00001);
    assertEqualEpsilon(-0.707107, a.getY(), 0.00001);
    assertEqualEpsilon(0.0, a.getZ(), 0.00001);

    a = UnitVector(Utils::degreesToRadians(270), 0.0);
    assertEqual(Utils::degreesToRadians(270), a.getDirection());
    assertEqual(0.0, a.getElevation());
    assertEqualEpsilon(-1.0, a.getX(), 0.00001);
    assertEqualEpsilon(0.0, a.getY(), 0.00001);
    assertEqualEpsilon(0.0, a.getZ(), 0.00001);

    a = UnitVector(Utils::degreesToRadians(315), 0.0);
    assertEqual(Utils::degreesToRadians(315), a.getDirection());
    assertEqual(0.0, a.getElevation());
    assertEqualEpsilon(-0.707107, a.getX(), 0.00001);
    assertEqualEpsilon(0.707107, a.getY(), 0.00001);
    assertEqualEpsilon(0.0, a.getZ(), 0.00001);

    a = Vector(a, 12345);
    assertEqual(Utils::degreesToRadians(315), a.getDirection());
    assertEqual(0.0, a.getElevation());
    assertEqualEpsilon(-0.707107, a.getX(), 0.00001);
    assertEqualEpsilon(0.707107, a.getY(), 0.00001);
    assertEqualEpsilon(0.0, a.getZ(), 0.00001);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
