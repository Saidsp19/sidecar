#include "UnitTest/UnitTest.h"
#include "Utils.h"

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("PowerOf2") {}
    void test();
};

void
Test::test()
{
    assertEqual(1, Utils::PowerOf2<0>::GetValue());
    assertEqual(2, Utils::PowerOf2<1>::GetValue());
    assertEqual(4, Utils::PowerOf2<2>::GetValue());
    assertEqual(65536, Utils::PowerOf2<16>::GetValue());
    assertEqual(1073741824, Utils::PowerOf2<30>::GetValue());
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
