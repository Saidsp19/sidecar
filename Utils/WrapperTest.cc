#include "Wrapper.h"
#include "UnitTest/UnitTest.h"

struct TestWrapper : public UnitTest::TestObj {
    TestWrapper() : UnitTest::TestObj("Pool") {}
    void test();
};

void
TestWrapper::test()
{
    {
        Utils::Wrapper w("this is a test", 2, 0, "> ");
        std::ostringstream os("");
        w.print(os);
        assertEqual("this \n> is \n> a \n> test ", os.str());
    }
    {
        std::ostringstream os("");
        os << Utils::wrap("this is another test", 10, 0, "");
        assertEqual("this is \nanother \ntest ", os.str());
    }
}

int
main(int argc, const char* argv[])
{
    return TestWrapper().mainRun();
}
