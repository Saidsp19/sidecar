#include "Messages/VMEHeader.h"
#include "Messages/Video.h"
#include "UnitTest/UnitTest.h"

#include "SynchronizedBuffer.h"

using namespace SideCar::Messages;
using namespace SideCar::Algorithms;

class SynchronizedBufferTest : public UnitTest::TestObj {
public:
    SynchronizedBufferTest() : TestObj("SynchronizedBuffer") {}

    void test();
};

void
SynchronizedBufferTest::test()
{
    SynchronizedBuffer<Video> buffer(3);
    assertEqual(size_t(3), buffer.getCapacity());
    assertEqual(size_t(0), buffer.getMaxMsgSize());
    assertFalse(buffer.full());
    assertTrue(buffer.empty());

    VMEDataMessage vme;
    vme.header.azimuth = 0;
    vme.header.pri = 10;
    {
        int16_t init[] = {1, 2, 3};
        Video::Ref msg(Video::Make("test", vme, init, init + 3));
        buffer.add(msg);
    }

    assertEqual(size_t(1), buffer.size());
    assertEqual(1, buffer[0][0]);
    assertEqual(size_t(3), buffer.getMaxMsgSize());
    assertFalse(buffer.full());
    assertFalse(buffer.empty());

    assertFalse(buffer.find(9).get());

    {
        ++vme.header.pri;
        int16_t init[] = {4, 5, 6, 7};
        Video::Ref msg(Video::Make("test", vme, init, init + 4));
        buffer.add(msg);
    }

    assertEqual(size_t(2), buffer.size());
    assertEqual(4, buffer[0][0]);
    assertEqual(1, buffer[1][0]);
    assertEqual(size_t(4), buffer.getMaxMsgSize());
    assertFalse(buffer.full());
    assertFalse(buffer.empty());

    assertTrue(buffer.find(11).get());
    assertEqual(size_t(0), buffer.size());

    {
        ++vme.header.pri;
        int16_t init[] = {8, 9};
        Video::Ref msg(Video::Make("test", vme, init, init + 2));
        buffer.add(msg);
    }

    assertEqual(size_t(1), buffer.size());
    assertEqual(8, buffer[0][0]);
    assertEqual(size_t(4), buffer.getMaxMsgSize());
    assertFalse(buffer.full());
    assertFalse(buffer.empty());

    {
        ++vme.header.pri;
        int16_t init[] = {10, 11, 12};
        Video::Ref msg(Video::Make("test", vme, init, init + 3));
        buffer.add(msg);
    }

    assertEqual(size_t(2), buffer.size());
    assertEqual(10, buffer[0][0]);
    assertEqual(8, buffer[1][0]);
    assertEqual(size_t(4), buffer.getMaxMsgSize());
    assertFalse(buffer.full());
    assertFalse(buffer.empty());

    buffer.clear();
    assertEqual(size_t(0), buffer.size());
    assertEqual(size_t(0), buffer.getMaxMsgSize());
    assertFalse(buffer.full());
    assertTrue(buffer.empty());
}

int
main(int argc, char** argv)
{
    return (new SynchronizedBufferTest)->mainRun();
}
