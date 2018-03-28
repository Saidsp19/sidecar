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

#include "NCIntegrate.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::ProcSuite<Test> {
    Test() : UnitTest::ProcSuite<Test>(this, "NCIntegrateTests")
    {
        add("normal", &Test::testNormal);
        add("iq", &Test::testIQ);
    }

    void testNormal();
    void testIQ();
};

void
Test::testNormal()
{
    Logger::Log& root = Logger::Log::Root();
    // root.setPriorityLimit(Logger::Priority::kDebug);
    Logger::ProcLog log("testNormal", root);

    static const int16_t kOffset = 32767 - 28;

    Utils::TemporaryFilePath testOutputPath("ncintegrateTestOutputNormal");
    {
        Stream::Ref stream(Stream::Make("testNormal"));

        assertEqual(0, stream->push(new ShutdownMonitorModule(stream)));

        FileWriterTaskModule* writer = new FileWriterTaskModule(stream);
        assertEqual(0, stream->push(writer));
        assertTrue(writer->getTask()->openAndInit("Video", testOutputPath));

        ControllerModule* controller = new ControllerModule(stream);
        assertEqual(0, stream->push(controller));
        assertTrue(controller->getTask()->openAndInit("NCIntegrate"));

        stream->put(ProcessingStateChangeRequest(ProcessingState::kRun).getWrapped());

        NCIntegrate* nci = dynamic_cast<NCIntegrate*>(controller->getTask()->getAlgorithm());
        assertTrue(nci);
        nci->setNumPulses(3);

        int16_t init[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (int index = 0; index < 5; ++index) {
            VMEDataMessage vme;
            vme.header.azimuth = index;

            // Mess with the first element to test proper calculations for very large values.
            //
            init[0] = index * 7 + kOffset;
            Video::Ref msg(Video::Make("test", vme, init, init + 10));
            MessageManager mgr(msg);
            stream->put(mgr.getMessage(), 0);
            assertFalse(mgr.hasEncoded());
        }

        stream->put(ShutdownRequest().getWrapped());
        ACE_Reactor::instance()->run_reactor_event_loop();
        ACE_Reactor::instance()->reset_reactor_event_loop();
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
        // (0 + 7 + 14) / 3 = 7
        //
        assertEqual(7 + kOffset, *pos++);
        assertEqual(2, *pos++);
        assertEqual(3, *pos++);
        assertEqual(4, *pos++);
        assertEqual(5, *pos++);
        assertEqual(6, *pos++);
        assertEqual(7, *pos++);
        assertEqual(8, *pos++);
        assertEqual(9, *pos++);
        assertEqual(10, *pos++);
        assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
        Decoder decoder(reader->getMessage());
        Video::Ref msg(decoder.decode<Video>());
        assertEqual(size_t(10), msg->size());
        Video::const_iterator pos = msg->begin();

        // (7 + 14 + 21) / 3 = 42 / 3 = 14
        //
        assertEqual(14 + kOffset, *pos++);
        assertEqual(2, *pos++);
        assertEqual(3, *pos++);
        assertEqual(4, *pos++);
        assertEqual(5, *pos++);
        assertEqual(6, *pos++);
        assertEqual(7, *pos++);
        assertEqual(8, *pos++);
        assertEqual(9, *pos++);
        assertEqual(10, *pos++);
        assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
        Decoder decoder(reader->getMessage());
        Video::Ref msg(decoder.decode<Video>());
        assertEqual(size_t(10), msg->size());
        Video::const_iterator pos = msg->begin();

        // (14 + 21 + 28) / 3 = 63 / 3 = 21
        //
        assertEqual(21 + kOffset, *pos++);
        assertEqual(2, *pos++);
        assertEqual(3, *pos++);
        assertEqual(4, *pos++);
        assertEqual(5, *pos++);
        assertEqual(6, *pos++);
        assertEqual(7, *pos++);
        assertEqual(8, *pos++);
        assertEqual(9, *pos++);
        assertEqual(10, *pos++);
        assertTrue(pos == msg->end());
    }

    assertFalse(reader->fetchInput());

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

void
Test::testIQ()
{
    Logger::Log& root = Logger::Log::Root();
    // root.setPriorityLimit(Logger::Priority::kDebug);
    Logger::ProcLog log("testIQ", root);

    static const int16_t kOffset = 32767 - 28;

    Utils::TemporaryFilePath testOutputPath("ncintegrateTestOutputIQ");
    {
        Stream::Ref stream(Stream::Make("testIQ"));

        assertEqual(0, stream->push(new ShutdownMonitorModule(stream)));

        FileWriterTaskModule* writer = new FileWriterTaskModule(stream);
        assertEqual(0, stream->push(writer));
        assertTrue(writer->getTask()->openAndInit("Video", testOutputPath));

        ControllerModule* controller = new ControllerModule(stream);
        assertEqual(0, stream->push(controller));
        assertTrue(controller->getTask()->openAndInit("NCIntegrate"));

        stream->put(ProcessingStateChangeRequest(ProcessingState::kRun).getWrapped());

        NCIntegrate* nci = dynamic_cast<NCIntegrate*>(controller->getTask()->getAlgorithm());
        assertTrue(nci);
        nci->setNumPulses(3);
        nci->setIQValues(true);

        int16_t init[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (int index = 0; index < 5; ++index) {
            VMEDataMessage vme;
            vme.header.azimuth = index;

            // Mess with the first two elements (I,Q) to test proper calculations for very large values.
            //
            init[0] = index * 7 + kOffset;
            init[1] = index * 7 + kOffset;
            Video::Ref msg(Video::Make("test", vme, init, init + 10));
            MessageManager mgr(msg);
            stream->put(mgr.getMessage(), 0);
            assertFalse(mgr.hasEncoded());
        }

        stream->put(ShutdownRequest().getWrapped());
        ACE_Reactor::instance()->run_reactor_event_loop();
        ACE_Reactor::instance()->reset_reactor_event_loop();
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
        LOGERROR << msg->dataPrinter() << std::endl;

        assertEqual(26701, *pos++);
        assertEqual(5, *pos++);
        assertEqual(8, *pos++);
        assertEqual(11, *pos++);
        assertEqual(13, *pos++);
        assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
        Decoder decoder(reader->getMessage());
        Video::Ref msg(decoder.decode<Video>());
        assertEqual(size_t(5), msg->size());
        Video::const_iterator pos = msg->begin();
        LOGERROR << msg->dataPrinter() << std::endl;

        assertEqual(26718, *pos++);
        assertEqual(5, *pos++);
        assertEqual(8, *pos++);
        assertEqual(11, *pos++);
        assertEqual(13, *pos++);
        assertTrue(pos == msg->end());
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
        Decoder decoder(reader->getMessage());
        Video::Ref msg(decoder.decode<Video>());
        assertEqual(size_t(5), msg->size());
        Video::const_iterator pos = msg->begin();
        LOGERROR << msg->dataPrinter() << std::endl;

        assertEqual(26735, *pos++);
        assertEqual(5, *pos++);
        assertEqual(8, *pos++);
        assertEqual(11, *pos++);
        assertEqual(13, *pos++);
        assertTrue(pos == msg->end());
    }

    assertFalse(reader->fetchInput());

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}
int
main(int argc, const char* argv[])
{
    vsip::vsipl v;
    return Test().mainRun();
}
