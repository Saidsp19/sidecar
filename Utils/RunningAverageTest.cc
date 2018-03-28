#include <stdlib.h>

#include "Logger/Log.h"
#include "RunningAverage.h"
#include "UnitTest/UnitTest.h"

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("RunningAverage") {}
    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    // Test the smallest running median size.
    //
    Utils::RunningAverage r1(1, 0.0);
    assertEqual(0.0, r1.getAverageValue());
    r1.addValue(8.0);
    assertEqual(8.0, r1.getAverageValue());
    r1.addValue(4.0);
    assertEqual(4.0, r1.getAverageValue());

    r1.setWindowSize(4, 4.0);
    assertEqual(4.0, r1.getAverageValue());
    r1.addValue(0.0);
    assertEqual(3.0, r1.getAverageValue());
    r1.addValue(0.0);
    assertEqual(2.0, r1.getAverageValue());
    r1.addValue(0.0);
    assertEqual(1.0, r1.getAverageValue());
    r1.addValue(0.0);
    assertEqual(0.0, r1.getAverageValue());
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
