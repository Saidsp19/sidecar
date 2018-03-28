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

#include "Offset.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("Offset") {}
    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Utils::TemporaryFilePath testOutputPath("offsetTestOutput");
    {
        Stream::Ref stream(Stream::Make("test"));

        assertEqual(0, stream->push(new ShutdownMonitorModule(stream)));

        FileWriterTaskModule* writer = new FileWriterTaskModule(stream);
        assertEqual(0, stream->push(writer));
        assertTrue(writer->getTask()->openAndInit("Video", testOutputPath));

        ControllerModule* controller = new ControllerModule(stream);
        assertEqual(0, stream->push(controller));
        assertTrue(controller->getTask()->openAndInit("Offset"));

        stream->put(ProcessingStateChangeRequest(ProcessingState::kRun).getWrapped());

        Offset* offset = dynamic_cast<Offset*>(controller->getTask()->getAlgorithm());
        assertTrue(offset);
        offset->setOffset(13);

        VMEDataMessage vme;
        vme.header.azimuth = 0;
        int16_t init[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        Video::Ref msg(Video::Make("test", vme, init, init + 10));
        MessageManager mgr(msg);
        stream->put(mgr.getMessage(), 0);
        assertFalse(mgr.hasEncoded());

        stream->put(ShutdownRequest().getWrapped());
        ACE_Reactor::instance()->run_reactor_event_loop();

        writer->getTask()->close(1);
        stream->close();
    }

    FileReader::Ref reader(new FileReader);
    ACE_FILE_Addr inputAddr(testOutputPath);
    ACE_FILE_Connector inputConnector(reader->getDevice(), inputAddr);
    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    Decoder decoder(reader->getMessage());
    {
        Video::Ref msg(decoder.decode<Video>());
        assertEqual(size_t(10), msg->size());
        Video::const_iterator pos = msg->begin();
        assertEqual(14, *pos++);
        assertEqual(15, *pos++);
        assertEqual(16, *pos++);
        assertEqual(17, *pos++);
        assertEqual(18, *pos++);
        assertEqual(19, *pos++);
        assertEqual(20, *pos++);
        assertEqual(21, *pos++);
        assertEqual(22, *pos++);
        assertEqual(23, *pos++);
        assertTrue(pos == msg->end());
    }

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
