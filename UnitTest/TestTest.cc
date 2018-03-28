#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "UnitTest.h"

struct TestTestObj : public UnitTest::TestObj {
    TestTestObj() : TestObj("UnitTest::TestObj") {}
    virtual void test();
};

void
TestTestObj::test()
{
    assertTrue(true);
}

struct TestMulti : public UnitTest::ProcSuite<TestMulti> {
    TestMulti() : UnitTest::ProcSuite<TestMulti>(this, "UnitTest::TestMulti")
    {
        add("assertTrue", &TestMulti::testTrue);
        add("assertFalse", &TestMulti::testFalse);
        add("assertEqual", &TestMulti::testEqual);
        add("error", &TestMulti::testError);
        add("unknown", &TestMulti::testUnknown);
    }

    void testTrue();
    void testFalse();
    void testEqual();
    void testError();
    void testUnknown();
};

void
TestMulti::testTrue()
{
    assertTrue(true);
    try {
        assertTrue(false);
        throw "expected assertTrue to throw";
    } catch (const UnitTest::UnitTestException&) {
        ;
    }
}

void
TestMulti::testFalse()
{
    assertFalse(false);
    try {
        assertFalse(true);
        throw "expected assertFalse to throw";
    } catch (const UnitTest::UnitTestException&) {
        ;
    }
}

void
TestMulti::testEqual()
{
    int a = -1;
    assertEqual(-1, a);
    assertNotEqual(-2, a);

    try {
        assertEqual(-2, a);
        assertTrue(false);
    } catch (const UnitTest::UnitTestException&) {
        ;
    }

    try {
        assertNotEqual(-1, a);
        assertTrue(false);
    } catch (const UnitTest::UnitTestException&) {
        ;
    }

    std::string c("abc");
    const char d[] = "abc";
    assertEqual("abc", c);
    assertEqual(c, "abc");
    assertEqual(c, d);
    assertEqual(d, c);
    assertEqual("abc", "abc");

    assertNotEqual("def", c);
    assertNotEqual(c, "def");
    const char e[] = "def";
    assertNotEqual(c, e);
    assertNotEqual(e, c);
    assertNotEqual("abc", "defg");

    const char* c1 = ::strdup("foo");
    char const c2[100] = {'f', 'o', 'o', 0};
    char* c3 = ::strdup("foo");
    char c4[100] = {'f', 'o', 'o', 0};
    std::string c5("foo");
    assertEqual(c1, c1);
    assertEqual(c1, c2);
    assertEqual(c1, c3);
    assertEqual(c1, c4);
    assertEqual(c1, c5);

    assertEqual(c2, c1);
    assertEqual(c2, c2);
    assertEqual(c2, c3);
    assertEqual(c2, c4);
    assertEqual(c2, c5);

    assertEqual(c3, c1);
    assertEqual(c3, c2);
    assertEqual(c3, c3);
    assertEqual(c3, c4);
    assertEqual(c3, c5);

    assertEqual(c4, c1);
    assertEqual(c4, c2);
    assertEqual(c4, c3);
    assertEqual(c4, c4);
    assertEqual(c4, c5);

    assertEqual(c5, c1);
    assertEqual(c5, c2);
    assertEqual(c5, c3);
    assertEqual(c5, c4);
    assertEqual(c5, c5);

    double b = 3.14159;
    assertEqual(3.14159, b);
    try {
        assertEqual(2.1818, b);
        assertTrue(false);
    } catch (const UnitTest::UnitTestException&) {
        ;
    }

    double b2 = b;
    b += 2 * std::numeric_limits<double>::epsilon();
    assertNotEqual(b2, b);
    try {
        assertEqual(b2, b);
        assertTrue(false);
    } catch (const UnitTest::UnitTestException&) {
        ;
    }

    double x = 1.0 / 3.0;
    assertEqual(1.0 / 3.0, x);
    assertEqualEpsilon(0.2, x, 0.15);
    assertNotEqualEpsilon(0.2, x, 0.01);
    try {
        assertEqualEpsilon(0.2, x, 0.10);
        assertTrue(false);
    } catch (const UnitTest::UnitTestException&) {
        ;
    }
}

void
TestMulti::testError()
{
    throw std::runtime_error("some kind of runtime error");
}

void
TestMulti::testUnknown()
{
    throw "some kind of unknown error";
}

int
main(int, const char**)
{
    // Create container to hold various test case objects.
    //
    UnitTest::Suite ts("TestSuite");
    ts.add(new TestTestObj);
    ts.add(new TestMulti);

    // Only return EXIT_SUCCESS if we match the exected number of bad tests.
    //
    UnitTest::RunResults rr;
    ts.run(rr);
    ts.mainRun(&std::cerr);

    const int expectedBad = 2;

    if (rr.numBad() == expectedBad) {
        std::cout << "\n--- UnitTest testing was successful - expected to see " << expectedBad << " errors\n";
    }

    return rr.numBad() == expectedBad ? EXIT_SUCCESS : EXIT_FAILURE;
}
