#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "Algorithms/ShutdownMonitor.h"
#include "IO/Readers.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/ShutdownRequest.h"
#include "IO/Stream.h"

#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "VideoInterpolation.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("VideoInterpolation") {}
    void test();
};

void
Test::test()
{
    // Logger::Log& log = Logger::Log::Find("SideCar.Algorithms");
    // log.setPriorityLimit(Logger::Priority::kDebug);

//    vsip::vsipl vpp;

    Utils::TemporaryFilePath testOutputPath("videoInterpolationTestOutput");
    {
	Stream::Ref stream(Stream::Make("test"));
	
	assertEqual(0, stream->push(new ShutdownMonitorModule(stream)));
	
	FileWriterTaskModule* writer = new FileWriterTaskModule(stream);
	assertEqual(0, stream->push(writer));
	assertTrue(writer->getTask()->openAndInit("Video", testOutputPath));

	ControllerModule* controller = new ControllerModule(stream);
	assertEqual(0, stream->push(controller));
	assertTrue(controller->getTask()->openAndInit("VideoInterpolation"));

	stream->put(ProcessingStateChangeRequest(
                       ProcessingState::kRun).getWrapped());

	VideoInterpolation* alg = dynamic_cast<VideoInterpolation*>(
	    controller->getTask()->getAlgorithm());
	assertTrue(alg);

	alg->setInterpolationCount(2);

	int16_t init[] = { 0, 0, 0, 3 };
	for (int index = 0; index < 2; ++index) {
	    VMEDataMessage vme;
	    vme.header.azimuth = index * 10;
	    init[0] = 10 - index * 5;
	    init[1] = 10 + index * 5;
	    init[2] = 0 + index * 10;
	    Video::Ref msg(Video::Make("test", vme, init, init + 4));
	    MessageManager mgr(msg);
	    stream->put(mgr.getMessage(), 0);
	    assertFalse(mgr.hasEncoded());
	}

	stream->put(ShutdownRequest().getWrapped());
	ACE_Reactor::instance()->run_reactor_event_loop();
	writer->getTask()->close(1);
    }

    FileReader::Ref reader(new FileReader);
    ACE_FILE_Addr inputAddr(testOutputPath);
    ACE_FILE_Connector inputConnector(reader->getDevice(), inputAddr);
    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
	// First message
	//
	Decoder decoder(reader->getMessage());
	Video::Ref msg(decoder.decode<Video>());
	assertEqual(size_t(4), msg->size());
	Video::const_iterator pos = msg->begin();
	assertEqual(10, *pos++);
	assertEqual(10, *pos++);
	assertEqual(0, *pos++);
	assertEqual(3, *pos++);
	assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
	// First interpolated message
	//
	Decoder decoder(reader->getMessage());
	Video::Ref msg(decoder.decode<Video>());
	assertEqual(size_t(4), msg->size());
	assertEqual(size_t(3), msg->getShaftEncoding());
	Video::const_iterator pos = msg->begin();
	assertEqual(8, *pos++);  // 10 - 5 * 1 / 3
	assertEqual(11, *pos++); // 10 + 5 * 1 / 3
	assertEqual(3, *pos++);  // 0 + 10 * 1 / 3
	assertEqual(3, *pos++);
	assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
	// Second interpolated message
	//
	Decoder decoder(reader->getMessage());
	Video::Ref msg(decoder.decode<Video>());
	assertEqual(size_t(4), msg->size());
	assertEqual(size_t(7), msg->getShaftEncoding());
	Video::const_iterator pos = msg->begin();
	assertEqual(6, *pos++);  // 10 - 5 * 2 / 3
	assertEqual(13, *pos++); // 10 + 5 * 2 / 3
	assertEqual(6, *pos++);  // 0 + 10 * 2 / 3
	assertEqual(3, *pos++);
	assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
	// Second message
	//
	Decoder decoder(reader->getMessage());
	Video::Ref msg(decoder.decode<Video>());
	assertEqual(size_t(4), msg->size());
	assertEqual(size_t(10), msg->getShaftEncoding());
	Video::const_iterator pos = msg->begin();
	assertEqual(5, *pos++);  // 10 - 5 * 3 / 3
	assertEqual(15, *pos++); // 10 + 5 * 3 / 3
	assertEqual(10, *pos++); // 0 + 10 * 3 / 3
	assertEqual(3, *pos++);
	assertTrue(pos == msg->end());
    }

    assertFalse(reader->fetchInput());

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
