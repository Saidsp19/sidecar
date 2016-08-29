#include <cmath>

#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

#include "SineCosineLUT.h"

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("SineCosineLUT") {}
    void test();
};

void
Test::test()
{
    Logger::Log& log(Logger::Log::Root());

    Utils::SineCosineLUT* lut = Utils::SineCosineLUT::Make(65536);
    assertTrue(lut);

    double epsilon = 1.0e-10;
    double sine, cosine;

    // Verify that we've calculated things correctly.
    //
    double increment = M_PI * 2.0 / 65536;
    for (size_t index = 0; index < 65536; ++index) {
	double angle = double(index) * increment;
	lut->lookup(index, sine, cosine);
	LOGDEBUG << index << ' ' << sine << ' ' << cosine << std::endl;
	assertEqualEpsilon(::sin(angle), sine, epsilon);
	assertEqualEpsilon(::cos(angle), cosine, epsilon);
    }
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
