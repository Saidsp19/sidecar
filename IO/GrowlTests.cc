#include "UnitTest/UnitTest.h"

#include "Growl.h"

using namespace SideCar;
using namespace SideCar::IO;

struct GrowlTests : public UnitTest::TestObj
{
    GrowlTests() : TestObj("Growl") {}
    void test();
};

void
GrowlTests::test()
{
    Growler growler("GrowlTests");
    growler.addNotification("enabledNotification");
    growler.addNotification("disabledNotification", false);
    growler.addNotification("foobarNotification");

    growler.addHost("localhost");
    assertTrue(growler.notify("enabledNotification", "Hi Mom",
                              "This is a test of the emergency broadcasting "
                              "system"));

    growler.useBroadcasting();
    assertTrue(growler.notify("enabledNotification", "Hi Dad",
                              "This is another test of the emergency "
                              "broadcasting system"));

    growler.addHost("localhost");
    assertTrue(growler.notify("foobarNotification", "Hi Sis",
                              "This is the last test of the emergency "
                              "broadcasting system"));

    assertFalse(growler.notify("unknownNotification", "Hi Bro",
                               "This is not a test of the emergency "
                               "broadcasting system"));
}

int
main(int, const char**)
{
    GrowlTests test;
    return test.mainRun();
}
