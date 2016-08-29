#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"

#include "Algorithms/ShutdownMonitor.h"
#include "IO/Readers.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/Module.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/ShutdownRequest.h"
#include "IO/Stream.h"

#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "OSCFAR.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("OSCFAR") {}
    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Logger::ProcLog log("test", Logger::Log::Root());

    const int kWindowSize = 5;
    const int kThresholdIndex = 2;
    const int kNumRows = 5;
    const int kNumCols = 20;
    int16_t inputData[kNumRows][kNumCols] = {
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1 },
	{ 1, 5, 9, 1, 5, 9, 1, 5, 9, 1, 5, 9, 1, 5, 9, 1, 5, 9, 1, 5 },
    };

    // NOTE: verify the following using MATLAB (I did it by hand).
    //
    int outputData[kNumRows][kNumCols] = {
	{ 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
	{ 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
	{ 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
	{ 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0 },
	{ 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0 },
    };

    // Create temp file to hold results of OSCFAR processing. Build processing stream consisting of a OSCFAR
    // algorithm, a FileWriter, and a ShutdownMontor. Then kNumRows messages to the algorithm. Finally, place a
    // ShutdownRequest message in the processing stream, and wait until the ACE event loop exits.
    //
    Utils::TemporaryFilePath testOutputPath("oscfarTestOutput");
    {
	Stream::Ref stream(Stream::Make("Blah"));
	assertEqual(0, stream->push(new ShutdownMonitorModule(stream)));

	FileWriterTaskModule* writer = new FileWriterTaskModule(stream);
	assertEqual(0, stream->push(writer));
	assertTrue(writer->getTask()->openAndInit("BinaryVideo",
                                                  testOutputPath));

	ControllerModule* controller = new ControllerModule(stream);
	assertEqual(0, stream->push(controller));
	assertTrue(controller->getTask()->openAndInit("OSCFAR"));
	stream->put(ProcessingStateChangeRequest(
                       ProcessingState::kRun).getWrapped());
	OSCFAR* alg = dynamic_cast<OSCFAR*>(
	    controller->getTask()->getAlgorithm());
	alg->setWindowSize(kWindowSize);
	alg->setThresholdIndex(kThresholdIndex);

	VMEDataMessage vme;
	vme.header.azimuth = 0;
	for (int index = 0; index < kNumRows; ++index) {
	    Video::Ref msg(Video::Make("test", vme, inputData[index],
                                       inputData[index] + kNumCols));
	    MessageManager mgr(msg);
	    stream->put(mgr.getMessage(), 0);
	    assertFalse(mgr.hasEncoded());
	}

	stream->put(ShutdownRequest().getWrapped());
	ACE_Reactor::instance()->run_reactor_event_loop();

	writer->getTask()->close(1);
	stream->close();
    }

    // Create a FileReader to read in the OSCFAR results from above. Read in all kNumRows messages, and compare
    // them with the expected results in outputData.
    //
    FileReader::Ref reader(new FileReader);
    ACE_FILE_Addr inputAddr(testOutputPath);
    ACE_FILE_Connector inputConnector(reader->getDevice(), inputAddr);
    for (int row = 0; row < kNumRows; ++row) {
	assertTrue(reader->fetchInput());
	assertTrue(reader->isMessageAvailable());
	Decoder decoder(reader->getMessage());
	BinaryVideo::Ref msg(decoder.decode<BinaryVideo>());
	LOGERROR << row << " " << msg << std::endl;
	BinaryVideo::const_iterator pos = msg->begin();
	for (int col = 0; col < kNumCols; ++col) {
	    assertEqual(outputData[row][col],
                        static_cast<int>(*pos++));
	}
    }

    // Should not have anything more to read.
    //
    assertFalse(reader->fetchInput());
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
