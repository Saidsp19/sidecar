#include <sstream>

#include "Format.h"
#include "UnitTest/UnitTest.h"

struct Test : public UnitTest::ProcSuite<Test> {
    Test() : UnitTest::ProcSuite<Test>(this, "FormatTests")
    {
        add("Format", &Test::testFormat);
        add("hexPtr", &Test::testHexPtr);
        add("formatTime", &Test::testFormatTime);
        add("center", &Test::testCenter);
        add("dashLine", &Test::testDashLine);
        add("dumpHex", &Test::testDumpHex);
        add("showErrno", &Test::testShowErrno);
    }

    void testFormat();
    void testHexPtr();
    void testFormatTime();
    void testCenter();
    void testDashLine();
    void testDumpHex();
    void testShowErrno();
};

void
Test::testFormat()
{
    std::ostringstream os("");
    os << std::dec << 123 << ' ' << Utils::Formatter<int>(Utils::Format().hex(), 123) << ' ' << 123
       << Utils::Formatter<double>(Utils::Format(8, 2), 1.2345) << ' ' << 123.45678;

    assertEqual("123 7b 123     1.2 123.457", os.str());
}

void
Test::testHexPtr()
{
    std::ostringstream os("");
    os << Utils::hexPtr((void*)12345) << ' ' << Utils::hexPtr(0);
    if (sizeof((void*)0) == 4)
        assertEqual("0x00003039 0x00000000", os.str());
    else
        assertEqual("0x0000000000003039 0x0000000000000000", os.str());
}

void
Test::testFormatTime()
{
    std::ostringstream os("");
    os << Utils::formatTime(12345.678);
    assertEqual(" 12345.7", os.str());
}

void
Test::testCenter()
{
    std::ostringstream os("");
    os << Utils::center(6, "abc");
    assertEqual(" abc  ", os.str());
}

void
Test::testDashLine()
{
    std::ostringstream os("");
    os << Utils::dashLine(6);
    assertEqual("------", os.str());
}

void
Test::testDumpHex()
{
    std::ostringstream os("");
    os << Utils::dumpHex("XYZ123", 6);
    assertEqual("58 59 5A 31 32 33 \n", os.str());
}

void
Test::testShowErrno()
{
    std::ostringstream os("");
    errno = 1;
    os << Utils::showErrno();
    assertEqual("Operation not permitted (1)", os.str());
}

int
main(int argc, char** argv)
{
    return Test().mainRun();
}
