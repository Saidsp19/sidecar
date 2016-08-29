#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"
#include "Utils/Utils.h"

#include "Vector.h"
#include "UnitVector.h"

using namespace SideCar::Geometry;

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("Vector") {}
    void test();
};

void
Test::test()
{
    Vector a;
    assertEqual(0.0, a.getX());
    assertEqual(0.0, a.getY());
    assertEqual(0.0, a.getZ());
    assertEqual(0.0, a.getMagnitude());
    assertEqual(0.0, a.getMagnitudeSquared());
    assertEqual(0.0, a.getDirection());
    assertEqual(0.0, a.getElevation());

    a = Vector(2.0, 3.0, 0.0);
    assertEqual(2.0, a.getX());
    assertEqual(3.0, a.getY());
    assertEqual(0.0, a.getZ());
    assertEqual(13.0, a.getMagnitudeSquared());
    assertEqual(::sqrt(13.0), a.getMagnitude());
    assertEqualEpsilon(33.6901, Utils::radiansToDegrees(a.getDirection()),
                       0.0001);
    assertEqual(0.0, a.getElevation());

    a += Vector(4.0, 5.0, 0.0);
    assertEqual(6.0, a.getX());
    assertEqual(8.0, a.getY());
    assertEqual(0.0, a.getZ());
    assertEqual(100.0, a.getMagnitudeSquared());
    assertEqual(::sqrt(100.0), a.getMagnitude());
    assertEqualEpsilon(36.8699, Utils::radiansToDegrees(a.getDirection()),
                       0.0001);
    assertEqual(0.0, a.getElevation());

    a -= Vector(4.0, 5.0, 0.0);
    assertEqual(2.0, a.getX());
    assertEqual(3.0, a.getY());
    assertEqual(0.0, a.getZ());
    assertEqual(13.0, a.getMagnitudeSquared());
    assertEqual(::sqrt(13.0), a.getMagnitude());
    assertEqualEpsilon(33.6901, Utils::radiansToDegrees(a.getDirection()),
                       0.0001);
    assertEqual(0.0, a.getElevation());

    a *= 1.0;
    assertEqual(2.0, a.getX());
    assertEqual(3.0, a.getY());
    assertEqual(0.0, a.getZ());
    assertEqual(13.0, a.getMagnitudeSquared());
    assertEqual(::sqrt(13.0), a.getMagnitude());
    assertEqualEpsilon(33.6901, Utils::radiansToDegrees(a.getDirection()),
                       0.0001);
    assertEqual(0.0, a.getElevation());

    a *= 2.0;
    assertEqual(4.0, a.getX());
    assertEqual(6.0, a.getY());
    assertEqual(0.0, a.getZ());
    assertEqual(52.0, a.getMagnitudeSquared());
    assertEqual(::sqrt(52.0), a.getMagnitude());
    assertEqualEpsilon(33.6901, Utils::radiansToDegrees(a.getDirection()),
                       0.0001);
    assertEqual(0.0, a.getElevation());

    a /= 2.0;
    assertEqual(2.0, a.getX());
    assertEqual(3.0, a.getY());
    assertEqual(0.0, a.getZ());
    assertEqual(13.0, a.getMagnitudeSquared());
    assertEqual(::sqrt(13.0), a.getMagnitude());
    assertEqualEpsilon(33.6901, Utils::radiansToDegrees(a.getDirection()),
                       0.0001);
    assertEqual(0.0, a.getElevation());

    a /= 1;
    assertEqual(2.0, a.getX());
    assertEqual(3.0, a.getY());
    assertEqual(0.0, a.getZ());
    assertEqual(13.0, a.getMagnitudeSquared());
    assertEqual(::sqrt(13.0), a.getMagnitude());
    assertEqualEpsilon(33.6901, Utils::radiansToDegrees(a.getDirection()),
                       0.0001);
    assertEqual(0.0, a.getElevation());
}


int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
