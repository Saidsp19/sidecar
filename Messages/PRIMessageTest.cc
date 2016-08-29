#include <cmath>
#include "ace/FILE_Connector.h"

#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Readers.h"
#include "IO/Writers.h"
#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "Video.h"

using namespace SideCar;
using namespace SideCar::Messages;
using namespace SideCar::Time;

struct Test : public UnitTest::TestObj
{
    Test() : TestObj("Video") {}

    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    VMEDataMessage vme;
    vme.header.msgDesc = (VMEHeader::kPackedReal << 16)
	| VMEHeader::kIRIGValidMask
	| VMEHeader::kTimeStampValidMask
	| VMEHeader::kAzimuthValidMask
	| VMEHeader::kPRIValidMask;
    vme.header.timeStamp = 0;
    vme.header.azimuth = 12345;
    vme.header.pri = 1;
    vme.header.irigTime = 1.2345;

    Video::Ref msg(Video::Make("Test::test()", vme, 3));
    assertEqual(0U, msg->size());
    assertEqual(12345U, msg->getRIUInfo().shaftEncoding);
    assertEqual(1U, msg->getRIUInfo().sequenceCounter);

    msg->push_back(0);
    assertEqual(1U, msg->size());
    msg->push_back(1);
    msg->push_back(2);
    msg->push_back(3);
    assertEqual(4U, msg->size());

    // Test iterators
    //
    Video::const_iterator pos = msg->begin();
    Video::const_iterator end = msg->end();
    assertFalse(pos == end);
    assertTrue(pos < end);
    assertTrue(pos <= end);
    assertTrue(end >= pos);
    assertTrue(end > pos);

    assertEqual(4, end - pos);
    assertEqual(0, *pos++);
    assertEqual(1, *pos++);
    assertEqual(2, *pos++);
    assertTrue(pos < end);
    assertEqual(3.0, *pos++);

    pos -= 4;
    assertEqual(1, *++pos);
    pos += 2;
    assertEqual(3, *pos);

    // Test indexing
    //
    assertEqual(0, msg[0]);
    assertEqual(1, msg[1]);
    msg[1] = 3;
    assertEqual(3, msg[1]);
    msg[1] = 1;
    assertEqual(2, msg[2]);
    assertEqual(3, msg[3]);
    
    // Test CDR streaming. First write PRIMessage object to a file.
    //
    Utils::TemporaryFilePath fp("primessageTestOutput");
    ACE_FILE_Addr addr(fp);
    {
	IO::FileWriter::Ref writer(IO::FileWriter::Make());
	ACE_FILE_Connector fd(writer->getDevice(), addr);
	IO::MessageManager mgr(msg);
	assertTrue(writer->write(mgr.getMessage()));
	vme.header.pri = 2;
	Video::Ref two(Video::Make("Test::test()", vme, 5));
	for (int count = 0; count < 5; ++count) two->push_back(1);

	assertEqual(12345U, two->getRIUInfo().shaftEncoding);
	assertEqual(2U, two->getRIUInfo().sequenceCounter);
	assertEqual(5U, two->size());

	IO::MessageManager mgr2(two);
	assertTrue(writer->write(mgr2.getMessage()));
    }

    // Now test reading PRIMessage object from a file.
    //
    {
	IO::FileReader::Ref reader(IO::FileReader::Make());
	ACE_FILE_Connector fd(reader->getDevice(), addr);
	assertFalse(reader->isMessageAvailable());
	assertTrue(reader->fetchInput());
	assertTrue(reader->isMessageAvailable());

	// Load a message in from the file.
	//
	{
	    IO::Decoder decoder(reader->getMessage());
	    msg = decoder.decode<Video>();
	}

	assertEqual(4U, msg->size());
	assertEqual(12345U, msg->getRIUInfo().shaftEncoding);
	assertEqual(1U, msg->getRIUInfo().sequenceCounter);

	pos = msg->begin();
	assertEqual(0, *pos++);
	assertEqual(1, *pos++);
	assertEqual(2, *pos++);
	assertEqual(3, *pos++);
	assertTrue(pos == msg->end());

	// Load second one.
	//
	assertTrue(reader->fetchInput());
	assertTrue(reader->isMessageAvailable());
	{
	    IO::Decoder decoder(reader->getMessage());
	    msg = decoder.decode<Video>();
	}
	assertEqual(5U, msg->size());
	assertEqual(12345U, msg->getRIUInfo().shaftEncoding);
	assertEqual(2U, msg->getRIUInfo().sequenceCounter);

	pos = msg->begin();
	assertEqual(1, *pos++);
	assertEqual(1, *pos++);
	assertEqual(1, *pos++);
	assertEqual(1, *pos++);
	assertEqual(1, *pos++);
	assertTrue(pos == msg->end());
    }
}

int
main(int, const char**)
{
    return Test().mainRun();
}
