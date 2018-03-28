#include <stdlib.h>

#include "Logger/Log.h"
#include "RunningMedian.h"
#include "UnitTest/UnitTest.h"

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("RunningMedian") {}
    void test();
};

void
Test::test()
{
    // Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    // Test the smallest running median size.
    //
    Utils::RunningMedian r1(1, 0.0);
    r1.dump();
    assertEqual(0.0, r1.getEstimatedMeanValue());
    assertEqual(8.0, r1.addValue(8.0));
    assertEqual(8.0, r1.getEstimatedMeanValue());
    assertEqual(8.0, r1.getMedianValue());
    assertEqual(4.0, r1.addValue(4.0));
    assertEqual(4.0, r1.getEstimatedMeanValue());
    assertEqual(4.0, r1.getMedianValue());
    assertEqual(4.0, r1.getMinimumValue());
    assertEqual(4.0, r1.getMaximumValue());

    // Test a two-element window
    //
    Utils::RunningMedian r2(2, 2.0);
    r2.dump();
    assertEqual(2.0, r2.getEstimatedMeanValue());
    assertEqual(2.0, r2.getMedianValue());
    assertEqual(5.0, r2.addValue(8.0));
    assertEqual(2.0, r2.getMinimumValue());
    assertEqual(8.0, r2.getMaximumValue());
    assertEqual(5.0, r2.getEstimatedMeanValue());
    assertEqual(5.0, r2.getMedianValue());
    assertEqual(6.0, r2.addValue(4.0));
    assertEqual(4.0, r2.getMinimumValue());
    assertEqual(8.0, r2.getMaximumValue());
    assertEqual(6.0, r2.getEstimatedMeanValue());
    assertEqual(6.0, r2.getMedianValue());
    assertEqual(6.0, r2.addValue(8.0));
    assertEqual(6.0, r2.getEstimatedMeanValue());
    assertEqual(6.0, r2.addValue(4.0));
    assertEqual(6.0, r2.getEstimatedMeanValue());
    assertEqual(6.0, r2.getMedianValue());
    assertEqual(3.0, r2.addValue(2.0));
    assertEqual(3.0, r2.getEstimatedMeanValue());
    assertEqual(3.0, r2.getMedianValue());
    assertEqual(2.0, r2.addValue(2.0));
    assertEqual(2.0, r2.getEstimatedMeanValue());
    assertEqual(2.0, r2.getMedianValue());
    assertEqual(2.0, r2.getMinimumValue());
    assertEqual(2.0, r2.getMaximumValue());

    // Test a three-element window
    //
    Utils::RunningMedian r3(3, 1.0);
    r3.dump();
    assertEqual(1.0, r3.getMedianValue());
    assertEqual(1.0, r3.getEstimatedMeanValue());
    assertEqual(1.0, r3.addValue(0.0));
    assertEqual(0.0, r3.addValue(-1.0));
    assertEqual(0.0, r3.addValue(0.0));
    assertEqual(0.0, r3.addValue(1.0));
    assertEqual(1.0, r3.addValue(2.0));
    assertEqual(1.0, r3.getEstimatedMeanValue());
    assertEqual(1.0, r3.getMedianValue());
    assertEqual(2.0, r3.addValue(9.0));
    assertEqual(4.0, r3.getEstimatedMeanValue());
    assertEqual(2.0, r3.getMedianValue());
    assertEqual(1.0, r3.getMinimumValue());
    assertEqual(9.0, r3.getMaximumValue());

    // Initialize running median to 0.0, with a window of 5 samples
    //
    Utils::RunningMedian r5(5, 0.0);
    r5.dump();
    assertEqual(0.0, r5.getEstimatedMeanValue());

    // Test adding to the end.
    //
    assertEqual(0.0, r5.addValue(1.0));
    assertEqual(0.0, r5.addValue(2.0));
    assertEqual(1.0, r5.addValue(3.0));
    assertEqual(2.0, r5.addValue(4.0));
    assertEqual(3.0, r5.addValue(5.0));
    assertEqual(3.0, r5.getEstimatedMeanValue());
    assertEqual(3.0, r5.getMedianValue());
    assertEqual(1.0, r5.getMinimumValue());
    assertEqual(5.0, r5.getMaximumValue());

    // Test adding to the beginning
    //
    assertEqual(3.0, r5.addValue(-1.0));
    assertEqual(3.0, r5.addValue(-1.0));
    assertEqual(-1.0, r5.addValue(-1.0));
    assertEqual(-1.0, r5.addValue(-1.0));
    assertEqual(-1.0, r5.addValue(-1.0));
    assertEqual(-1.0, r5.getEstimatedMeanValue());
    assertEqual(-1.0, r5.getMedianValue());
    assertEqual(-1.0, r5.getMinimumValue());
    assertEqual(-1.0, r5.getMaximumValue());

    // Test adding in the middle
    //
    assertEqual(-1.0, r5.addValue(-2.0));
    assertEqual(-1.0, r5.addValue(-1.0));
    assertEqual(-1.0, r5.addValue(2.0));
    assertEqual(-1.0, r5.addValue(1.0));
    assertEqual(-1.0, r5.addValue(-1.0));
    assertEqual(1.0, r5.addValue(1.0));
    assertEqual(1.0, r5.addValue(0.0));
    assertEqual(0.0, r5.addValue(0.0));
    assertEqual(0.0, r5.addValue(0.0));
    assertEqual(0.0, r5.addValue(0.0));
    assertEqual(0.0, r5.addValue(0.0));
    assertEqual(0.0, r5.getEstimatedMeanValue());
    assertEqual(0.0, r5.getMedianValue());

    r5.clear(10.0);
    assertEqual(10.0, r5.getEstimatedMeanValue());
    assertEqual(10.0, r5.getMedianValue());
    assertEqual(10.0, r5.getMinimumValue());
    assertEqual(10.0, r5.getMaximumValue());

    assertEqual(10.0, r5.addValue(0.0));
    assertEqual(8.0, r5.getEstimatedMeanValue());
    assertEqual(10.0, r5.getMedianValue());
    assertEqual(10.0, r5.addValue(9.0));
    assertEqual(9.0, r5.addValue(1.0));
    assertEqual(8.0, r5.addValue(8.0));
    assertEqual(5.6, r5.getEstimatedMeanValue());
    assertEqual(8.0, r5.getMedianValue());
    assertEqual(0.0, r5.getMinimumValue());
    assertEqual(10.0, r5.getMaximumValue());

    Utils::RunningMedian::OrderedIndex index = r5.generateOrderedIndex(4);
    assertEqual(10.0, r5.getOrderedValue(index));
    assertEqual(9.0, r5.getOrderedValue(3));

    // Initialize running median to 0.0, with a window of 20 samples
    //
    Utils::RunningMedian r20(20, 0.0);
    r20.dump();
    ::srandom(0);
    for (int count = 0; count < 80; ++count) r20.addValue(::random() % 10);
    assertEqualEpsilon(5.0, r20.getMedianValue(), 3.0);

    // Initialize running median to 0.0, with a window of 50 samples
    //
    Utils::RunningMedian r50(50, 0.0);
    r20.dump();

    // Fill with random values. This is really a test that generates tons of debug log output to help identify
    // problems with 'finger' maintenance.
    //
    ::srandom(0);
    for (int count = 0; count < 500; ++count) r50.addValue(::random() % 100);
    assertEqualEpsilon(50.0, r50.getMedianValue(), 25.0);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
