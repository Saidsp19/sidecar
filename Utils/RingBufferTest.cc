#include "RingBuffer.h"
#include "UnitTest/UnitTest.h"

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("RingBuffer") {}
    void test();
};

struct Foo {
    Foo(const std::string& v) : value(v) {}
    std::string value;
};

void
Test::test()
{
    Utils::PtrRingBuffer<Foo> rb(7);
    assertEqual(7, rb.getCapacity());
    assertEqual(0, rb.getSize());
    rb.pushOn(new Foo("one"));
    assertFalse(rb.isEmpty());
    assertFalse(rb.isFull());
    assertEqual(1, rb.getSize());
    Foo* z = rb.popOff();
    assertEqual(std::string("one"), z->value);
    assertTrue(rb.isEmpty());
    assertFalse(rb.isFull());
    assertEqual(0, rb.getSize());
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
