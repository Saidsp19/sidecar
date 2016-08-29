#include "UnitTest/UnitTest.h"

#include "CLUT.h"

using namespace SideCar::GUI;

struct Test : public UnitTest::TestObj
{
    Test() : TestObj("CLUT") {}

    void test();
};

void
Test::test()
{
    static double kEpsilon = 1.0E-6;

    // Check out the values of RGB655
    //
    CLUT clut(CLUT::kRGB655);

    // Smallest sample value (-32768) in 655 binary is 100000 00000 00000
    //
    Color color = clut.getColor(0.0);
    assertEqualEpsilon(32.0 / 63.0, color.red, kEpsilon);
    assertEqual(0.0, color.green);
    assertEqual(0.0, color.blue);

    // Middle sample value (0) in 655 binary is 000000 00000 00000
    //
    color = clut.getColor(0.5);
    assertEqual(0.0, color.red);
    assertEqual(0.0, color.green);
    assertEqual(0.0, color.blue);

    // Sample value 1 in 655 binary should only increment blue.
    //
    color = clut.getColor(0.5 + 1.125 / 65535.0);
    assertEqual(0.0, color.red);
    assertEqual(0.0, color.green);
    assertEqualEpsilon(1.0 / 31.0, color.blue, kEpsilon);

    // Let's see if we can get -1 sample value -- all bits 1.
    //
    color = clut.getColor(0.5 - 1.0 / 65536.0);
    assertEqual(1.0, color.red);
    assertEqual(1.0, color.green);
    assertEqual(1.0, color.blue);

    // Largest sample value (32767) in 655 binary is 011111 11111 11111
    //
    color = clut.getColor(1.0);
    assertEqualEpsilon(31.0 / 63.0, color.red, kEpsilon);
    assertEqual(1.0, color.green);
    assertEqual(1.0, color.blue);

    // Check out the values of RGB565
    //
    clut.setType(CLUT::kRGB565);

    // Smallest sample value (-32768) in 565 binary is 10000 000000 00000
    //
    color = clut.getColor(0.0);
    assertEqualEpsilon(16.0 / 31.0, color.red, kEpsilon);
    assertEqual(0.0, color.green);
    assertEqual(0.0, color.blue);

    // Middle sample value (0) in 565 binary is 00000 000000 00000
    //
    color = clut.getColor(0.5);
    assertEqual(0.0, color.red);
    assertEqual(0.0, color.green);
    assertEqual(0.0, color.blue);

    // Let's see if we can get -1 sample value -- all bits 1.
    //
    color = clut.getColor(0.5 - 1.0 / 65536.0);
    assertEqual(1.0, color.red);
    assertEqual(1.0, color.green);
    assertEqual(1.0, color.blue);

    // Largest sample value (32767) in 565 binary is 01111 111111 11111
    //
    color = clut.getColor(1.0);
    assertEqualEpsilon(15.0 / 31.0, color.red, kEpsilon);
    assertEqual(1.0, color.green);
    assertEqual(1.0, color.blue);

    // Check out the values of RGB556
    //
    clut.setType(CLUT::kRGB556);

    // Smallest sample value (-32768) in 556 binary is 10000 00000 000000
    //
    color = clut.getColor(0.0);
    assertEqualEpsilon(16.0 / 31.0, color.red, kEpsilon);
    assertEqual(0.0, color.green);
    assertEqual(0.0, color.blue);

    // Middle sample value (0) in 556 binary is 00000 00000 000000
    //
    color = clut.getColor(0.5);
    assertEqual(0.0, color.red);
    assertEqual(0.0, color.green);
    assertEqual(0.0, color.blue);

    // Let's see if we can get -1 sample value -- all bits 1.
    //
    color = clut.getColor(0.5 - 1.0 / 65536.0);
    assertEqual(1.0, color.red);
    assertEqual(1.0, color.green);
    assertEqual(1.0, color.blue);

    // Largest sample value (32767) in 556 binary is 01111 11111 111111
    //
    color = clut.getColor(1.0);
    assertEqualEpsilon(15.0 / 31.0, color.red, kEpsilon);
    assertEqual(1.0, color.green);
    assertEqual(1.0, color.blue);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
