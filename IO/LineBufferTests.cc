#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"
#include "LineBuffer.h"

using namespace SideCar::IO;

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("LineBuffer") {}
    void test();
};

void
Test::test()
{
    std::istringstream iss("#first\nsecond third\n\nfourth\n#last\n");
    LineBufferStream is(iss.rdbuf());
    assertEqual(0, is.getLineNumber());
    std::string token;

    assertTrue(is >> token);
    assertEqual("second", token);
    assertEqual(2, is.getLineNumber());

    std::ostringstream os;
    is.printContext(os);
    assertEqual("Line: 2 'second third'", os.str());

    assertTrue(is >> token);
    assertEqual("third", token);
    assertEqual(2, is.getLineNumber());

    assertTrue(is >> token);
    assertEqual("fourth", token);
    assertEqual(4, is.getLineNumber());

    token = "";
    assertFalse(is >> token);
    assertEqual("", token);

    Utils::TemporaryFilePath path("./blah");
    std::ofstream ofs(path.filePath());
    ofs << "#first\n"
	<< "second third\n"
	<< "fourth\n"
	<< "#last\n";
    ofs.close();

    LineBufferFile lbf(path.getFilePath().filePath());
    assertTrue(lbf);
    assertTrue(lbf >> token);
    assertEqual("second", token);
    assertEqual(2, lbf.getLineNumber());

    Utils::Exception ex("Error -");
    ex << lbf;
    assertEqual("Error - Line: 2 File: './blah'", ex.what());
}
    
int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
