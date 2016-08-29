#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

#include "CircularBuffer.h"

using namespace SideCar::Messages;

class CircularBufferTest : public UnitTest::TestObj
{
public:
    CircularBufferTest() : TestObj("CircularBuffer") {}

    void test();

private:
};

struct Counter
{
    int counter;
    Counter() :counter(0) {}
    void operator()(const Video::Ref& msg) { ++counter; }
};

void
CircularBufferTest::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    VideoCircularBuffer* buffer = VideoCircularBuffer::Make("hello");
    assertTrue(buffer);
    try {
	VideoCircularBuffer::Make("hello");
	assertTrue(false);;
    }
    catch (...) {
	;
    }

    try {
	VideoCircularBuffer::Get("blah");
	assertTrue(false);;
    }
    catch (...) {
	;
    }

    assertEqual(buffer, VideoCircularBuffer::Get("hello"));

    VMEDataMessage vme;
    vme.header.msgDesc = (VMEHeader::kPackedReal << 16)
	| VMEHeader::kIRIGValidMask
	| VMEHeader::kTimeStampValidMask
	| VMEHeader::kAzimuthValidMask
	| VMEHeader::kPRIValidMask;
    vme.header.timeStamp = 0;
    vme.header.azimuth = 0;
    vme.header.pri = 1;
    vme.header.irigTime = 1.2345;
    vme.rangeMin = 0.0;
    vme.rangeFactor = 1.0;

    Video::Ref msg(Video::Make("CacheTests", vme, 0));
    buffer->add(msg);
    VideoCircularBuffer::const_iterator pos = buffer->locate(msg);
    assertTrue(pos.atNewest());
    ++pos;
    assertFalse(pos.atNewest());
    --pos;
    assertTrue(pos.atNewest());
    assertEqual(msg->getShaftEncoding(), pos->getShaftEncoding());
    assertEqual(1U, buffer->newest()->getRIUInfo().sequenceCounter);

    ++vme.header.pri;
    ++vme.header.azimuth;		// 1
    buffer->add(Video::Make("CacheTests", vme, 0));
    ++pos;
    assertEqual(2U, pos->getRIUInfo().sequenceCounter);
    assertEqual(1U, buffer->oldest()->getRIUInfo().sequenceCounter);
    assertEqual(2U, buffer->newest()->getRIUInfo().sequenceCounter);

    ++vme.header.pri;
    ++vme.header.azimuth;		// 2
    buffer->add(Video::Make("CacheTests", vme, 0));
    ++pos;
    assertEqual(3U, pos->getRIUInfo().sequenceCounter);
    assertEqual(1U, buffer->oldest()->getRIUInfo().sequenceCounter);
    assertEqual(3U, buffer->newest()->getRIUInfo().sequenceCounter);

    ++vme.header.pri;
    vme.header.azimuth = 0;	// 3
    buffer->add(Video::Make("CacheTests", vme, 0));
    ++pos;
    assertEqual(4U, pos->getRIUInfo().sequenceCounter);
    assertEqual(2U, buffer->oldest()->getRIUInfo().sequenceCounter);
    assertEqual(4U, buffer->newest()->getRIUInfo().sequenceCounter);

    // The following should fail if enabled.
    //
#if 0
    delete buffer;
#endif
}

int
main(int argc, const char* argv[])
{
    return (new CircularBufferTest)->mainRun();
}
