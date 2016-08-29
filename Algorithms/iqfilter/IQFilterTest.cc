#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "Algorithms/ShutdownMonitor.h"
#include "IO/Readers.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/ParametersChangeRequest.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/RecipientList.h"
#include "IO/ShutdownRequest.h"
#include "IO/Stream.h"

#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"
#include "XMLRPC/XmlRpcValue.h"

#include "IQFilter.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("IQFilter") {}
    void test();
};

void
Test::test()
{
    // Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Utils::TemporaryFilePath testOutputPath("iqfilterTestOutput");
    {
	Stream::Ref stream(Stream::Make("test"));
	
	assertEqual(0, stream->push(new ShutdownMonitorModule(stream)));
	
	FileWriterTaskModule* writer = new FileWriterTaskModule(stream);
	assertEqual(0, stream->push(writer));
	assertTrue(writer->getTask()->openAndInit("Video", testOutputPath));

	ControllerModule* controller = new ControllerModule(stream);
	assertEqual(0, stream->push(controller));
	assertTrue(controller->getTask()->openAndInit("IQFilter"));

	stream->put(ProcessingStateChangeRequest(
                       ProcessingState::kRun).getWrapped());

	IQFilter* alg = dynamic_cast<IQFilter*>(
	    controller->getTask()->getAlgorithm());
	assertTrue(alg);

	XmlRpc::XmlRpcValue parameterChange;
	parameterChange.setSize(2);
	parameterChange[0] = "filter";
	parameterChange[1] = 0;
	int16_t init[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	for (int index = 0; index < IQFilter::kNumFilterTypes; ++index) {
	    parameterChange[1] = index;
	    controller->getTask()->injectControlMessage(
		ParametersChangeRequest(parameterChange, false));
	    VMEDataMessage vme;
	    vme.header.azimuth = index;
	    Video::Ref msg(Video::Make("test", vme, init, init + 10));
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
	Decoder decoder(reader->getMessage());
	Video::Ref msg(decoder.decode<Video>());
	assertEqual(size_t(5), msg->size());
	Video::const_iterator pos = msg->begin();
	assertEqual(::rintf(10 * ::log10f(::sqrtf(1 + 4))), *pos++);
	assertEqual(::rintf(10 * ::log10f(::sqrtf(9 + 16))), *pos++);
	assertEqual(::rintf(10 * ::log10f(::sqrtf(25 + 36))), *pos++);
	assertEqual(::rintf(10 * ::log10f(::sqrtf(49 + 64))), *pos++);
	assertEqual(::rintf(10 * ::log10f(::sqrtf(81 + 100))), *pos++);
	assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
	Decoder decoder(reader->getMessage());
	Video::Ref msg(decoder.decode<Video>());
	assertEqual(size_t(5), msg->size());
	Video::const_iterator pos = msg->begin();
	assertEqual(::rintf(::sqrtf(1 + 4)), *pos++);
	assertEqual(::rintf(::sqrtf(9 + 16)), *pos++);
	assertEqual(::rintf(::sqrtf(25 + 36)), *pos++);
	assertEqual(::rintf(::sqrtf(49 + 64)), *pos++);
	assertEqual(::rintf(::sqrtf(81 + 100)), *pos++);
	assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
	Decoder decoder(reader->getMessage());
	Video::Ref msg(decoder.decode<Video>());
	assertEqual(size_t(5), msg->size());
	Video::const_iterator pos = msg->begin();
	assertEqual(1 + 4, *pos++);
	assertEqual(9 + 16, *pos++);
	assertEqual(25 + 36, *pos++);
	assertEqual(49 + 64, *pos++);
	assertEqual(81 + 100, *pos++);
	assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
	Decoder decoder(reader->getMessage());
	Video::Ref msg(decoder.decode<Video>());
	assertEqual(size_t(5), msg->size());
	Video::const_iterator pos = msg->begin();
	assertEqual(1, *pos++);
	assertEqual(3, *pos++);
	assertEqual(5, *pos++);
	assertEqual(7, *pos++);
	assertEqual(9, *pos++);
	assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
	Decoder decoder(reader->getMessage());
	Video::Ref msg(decoder.decode<Video>());
	assertEqual(size_t(5), msg->size());
	Video::const_iterator pos = msg->begin();
	assertEqual(2, *pos++);
	assertEqual(4, *pos++);
	assertEqual(6, *pos++);
	assertEqual(8, *pos++);
	assertEqual(10, *pos++);
	assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
	Decoder decoder(reader->getMessage());
	Video::Ref msg(decoder.decode<Video>());
	assertEqual(size_t(5), msg->size());
	Video::const_iterator pos = msg->begin();
	assertEqual(::rint(::atan2f(1, 2) * 2000.0), *pos++);
	assertEqual(::rint(::atan2f(3, 4) * 2000.0), *pos++);
	assertEqual(::rint(::atan2f(5, 6) * 2000.0), *pos++);
	assertEqual(::rint(::atan2f(7, 8) * 2000.0), *pos++);
	assertEqual(::rint(::atan2f(9, 10) * 2000.0), *pos++);
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
