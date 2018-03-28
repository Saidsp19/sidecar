#include <algorithm>
#include <iterator>

#include "QuickSelect.h"
#include "UnitTest/UnitTest.h"

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("QuickSelect") {}

    void test();
};

void
Test::test()
{
    // Create an array of random integer values.
    //
    const size_t count = 10000;
    std::vector<int> a;
    for (size_t index = 0; index < count; ++index) a.push_back(::random() % count);

    // Create a duplicate of the above array, and sort it using std::sort.
    //
    std::vector<int> b(a);
    std::sort(b.begin(), b.end());

    // Use QuickSelect to obtain the median value from the unsorted elements.
    //
    int median = ::Utils::QuickSelect(a.begin(), a.end(), count / 2);
    std::clog << "median: " << median << std::endl;

    // Should be the same as the median from the sorted ones.
    //
    assertEqual(b[count / 2], median);

    // Now fill the array with the same value. Just for fun.
    //
    a.clear();
    for (size_t index = 0; index < count; ++index) a.push_back(1);
    median = ::Utils::QuickSelect(a.begin(), a.end(), count / 2);
    std::clog << "median: " << median << std::endl;

    assertEqual(1, median);

    // Just lots of zeros and ones.
    //
    a.clear();
    for (size_t index = 0; index < count; ++index) a.push_back(::random() % 2);
    b = a;
    std::sort(b.begin(), b.end());

    median = ::Utils::QuickSelect(a.begin(), a.end(), count / 2);
    std::clog << "median: " << median << std::endl;

    assertEqual(b[count / 2], median);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
