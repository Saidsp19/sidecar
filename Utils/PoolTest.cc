#include "Pool.h"
#include "UnitTest/UnitTest.h"

struct Small {
    Small(char z) : a(z) {}

    static void* operator new(size_t size);
    static void operator delete(void* ptr, size_t size);

    char a;
    static Utils::Pool pool_;
};

Utils::Pool Small::pool_(sizeof(Small), 3);

void*
Small::operator new(size_t size)
{
    return pool_.allocate(size);
}

void
Small::operator delete(void* ptr, size_t size)
{
    pool_.release(ptr, size);
}

struct Large {
    Large(int w, double x, bool y, float z) : a(w), b(x), c(y), d(z) {}

    int a;
    double b;
    bool c;
    float d;
    static Utils::Pool pool_;

    static void* operator new(size_t size);
    static void operator delete(void* ptr, size_t size);
};

Utils::Pool Large::pool_(sizeof(Large), 5);

void*
Large::operator new(size_t size)
{
    return pool_.allocate(size);
}

void
Large::operator delete(void* ptr, size_t size)
{
    pool_.release(ptr, size);
}

struct TestPool : public UnitTest::ProcSuite<TestPool> {
    TestPool() : UnitTest::ProcSuite<TestPool>(this, "Pool")
    {
        add("Small", &TestPool::testSmall);
        add("Large", &TestPool::testLarge);
    }

    void testSmall();
    void testLarge();
};

void
TestPool::testSmall()
{
    // Nothing should be allocated right now.
    //
    assertEqual(size_t(0), Small::pool_.numFree());
    assertEqual(size_t(0), Small::pool_.numAllocated());
    assertEqual(size_t(0), Small::pool_.numBlocks());

    // This should cause the allocator to allocate a new block to hold 3 Small objects.
    //
    Small* x = new Small('x');
    assertEqual('x', x->a);
    Small* y = new Small('y');
    assertEqual('x', x->a);
    assertEqual('y', y->a);
    Small* z = new Small('z');
    assertEqual('x', x->a);
    assertEqual('y', y->a);
    assertEqual('z', z->a);

    // Should be all out of free slots.
    //
    assertEqual(size_t(0), Small::pool_.numFree());

    // Free the first slot.
    //
    delete x;
    assertEqual('y', y->a);
    assertEqual('z', z->a);
    assertEqual(size_t(1), Small::pool_.numFree());

    // Allocate another object. Should take the free slot
    //
    Small* a = new Small('a');
    assertEqual(x, a);
    assertEqual('a', a->a);
    assertEqual('y', y->a);
    assertEqual('z', z->a);
    assertEqual(size_t(0), Small::pool_.numFree());

    // Allocate another object. This should create a new block
    //
    Small* b = new Small('b');
    assertEqual('a', a->a);
    assertEqual('y', y->a);
    assertEqual('z', z->a);
    assertEqual(size_t(2), Small::pool_.numFree());
    assertEqual(size_t(6), Small::pool_.numAllocated());
    assertEqual(size_t(2), Small::pool_.numBlocks());

    for (int i = 0; i < 10000; ++i) new Small('z');

    assertEqual(size_t(1), Small::pool_.numFree());
    assertEqual(size_t(10005), Small::pool_.numAllocated());
    assertEqual(size_t(3335), Small::pool_.numBlocks());
    delete b;
}

void
TestPool::testLarge()
{
    // Create new Large object.
    //
    Large* x = new Large(123, 3.1415926, false, 123.45 / 67.89);
    assertEqual(123, x->a);
    assertEqualEpsilon(3.1415926, x->b, 0.00001);
    assertEqual(false, x->c);
    assertEqualEpsilon(123.45 / 67.89, x->d, 0.00001);
    assertEqual(size_t(4), Large::pool_.numFree());
    assertEqual(size_t(5), Large::pool_.numAllocated());
    assertEqual(size_t(1), Large::pool_.numBlocks());

    // Create another one
    //
    Large* y = new Large(456, 9.87654, true, 27.0 / 11.0);
    assertEqual(123, x->a);
    assertEqualEpsilon(3.1415926, x->b, 0.00001);
    assertEqual(false, x->c);
    assertEqualEpsilon(123.45 / 67.89, x->d, 0.00001);
    assertEqual(456, y->a);
    assertEqualEpsilon(9.87654, y->b, 0.00001);
    assertEqual(true, y->c);
    assertEqualEpsilon(27.0 / 11.0, y->d, 0.00001);
}

int
main(int argc, const char* argv[])
{
    return TestPool().mainRun();
}
