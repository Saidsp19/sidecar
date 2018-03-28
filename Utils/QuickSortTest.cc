#include <algorithm>
#include <iterator>

#include "QuickSort.h"
#include "UnitTest/UnitTest.h"

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("QuickSort") {}

    void test();
};

void
Test::test()
{
    {
        std::vector<int> a;
        for (size_t index = 0; index < 2000; ++index) a.push_back(::random() % 100);

        std::vector<int> b(a);
        std::sort(b.begin(), b.end());

        std::clog << "b: ";
        std::copy(b.begin(), b.end(), std::ostream_iterator<int>(std::clog, ","));
        std::clog << std::endl;

        ::Utils::QuickSort(a.begin(), a.end());
        std::clog << "a: ";
        std::copy(a.begin(), a.end(), std::ostream_iterator<int>(std::clog, ","));
        std::clog << std::endl;

        assertTrue(a == b);
    }
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
