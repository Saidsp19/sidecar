#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "Algorithms/ShutdownMonitor.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/Readers.h"
#include "IO/ShutdownRequest.h"
#include "IO/Stream.h"

#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "MTI2.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("MTI2") {}
    void test();
};

void
Test::test()
{
    // Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Utils::TemporaryFilePath testOutputPath("mti2TestOutput");
    {
        Stream::Ref stream(Stream::Make("test"));

        assertEqual(0, stream->push(new ShutdownMonitorModule(stream)));

        FileWriterTaskModule* writer = new FileWriterTaskModule(stream);
        assertEqual(0, stream->push(writer));
        assertTrue(writer->getTask()->openAndInit("Video", testOutputPath));

        ControllerModule* controller = new ControllerModule(stream);
        assertEqual(0, stream->push(controller));
        assertTrue(controller->getTask()->openAndInit("MTI2"));

        stream->put(ProcessingStateChangeRequest(ProcessingState::kRun).getWrapped());

        MTI2* mti2 = dynamic_cast<MTI2*>(controller->getTask()->getAlgorithm());
        assertTrue(mti2);

        int16_t init[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (int index = 0; index < 3; ++index) {
            init[0] = int16_t(::pow(3, index));
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
        assertEqual(size_t(10), msg->size());
        Video::const_iterator pos = msg->begin();
        // (3 - 1) * ::sqrt(0.5) = 1.414
        //
        assertEqual(1, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
        Decoder decoder(reader->getMessage());
        Video::Ref msg(decoder.decode<Video>());
        assertEqual(size_t(10), msg->size());
        Video::const_iterator pos = msg->begin();

        // (9 - 3) * ::sqrt(0.5) = 4.243
        //
        assertEqual(4, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
        assertEqual(0, *pos++);
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
