#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"

#include "IO/Decoder.h"
#include "IO/FileReaderTask.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"

#include "Algorithms/Controller.h"
#include "Algorithms/ShutdownMonitor.h"
#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("SandBox") {}
    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    Utils::TemporaryFilePath testInputPath("SandBoxTestInput");
    Utils::TemporaryFilePath testOutputPath("SandBoxTestOutput");

    {
        // Write out two messages to a file.
        //
        VMEDataMessage vme;
        vme.header.azimuth = 0;
        FileWriter::Ref writer(new FileWriter);
        ACE_FILE_Addr addr(testInputPath);
        ACE_FILE_Connector connector(writer->getDevice(), addr);

        {
            short init[] = {1, 2, 3};
            Video::Ref msg(Video::Make("test", vme, init, init + 3));
            assertTrue(writer->write(MessageManager(msg).getMessage()));
        }
        {
            short init[] = {4, 5, 6, 7, 8};
            Video::Ref msg(Video::Make("test", vme, init, init + 5));
            assertTrue(writer->write(MessageManager(msg).getMessage()));
        }
    }

    {
        Stream::Ref stream(Stream::Make("test"));

        assertEqual(0, stream->push(new ShutdownMonitorModule(stream)));

        FileWriterTaskModule* writerMod = new FileWriterTaskModule(stream);
        assertEqual(0, stream->push(writerMod));
        FileWriterTask::Ref writer = writerMod->getTask();
        writer->setTaskIndex(2);
        assertTrue(writer->openAndInit("Video", testOutputPath));

        ControllerModule* controllerMod = new ControllerModule(stream);
        assertEqual(0, stream->push(controllerMod));
        Controller::Ref controller = controllerMod->getTask();
        controller->setTaskIndex(1);

        controller->addOutputChannel(Channel(controller.get(), "one", "Video"));
        controller->addOutputChannel(Channel(controller.get(), "two", "Video"));
        controller->addOutputChannel(Channel(controller.get(), "three", "Video", writer.get(), 0));
        controller->addOutputChannel(Channel(controller.get(), "four", "Video"));

        FileReaderTaskModule* readerMod = new FileReaderTaskModule(stream);
        assertEqual(0, stream->push(readerMod));
        FileReaderTask::Ref reader = readerMod->getTask();
        reader->setTaskIndex(0);

        controller->addInputChannel(Channel(reader.get(), "one", "Video"));
        controller->addInputChannel(Channel(reader.get(), "two", "Video"));
        controller->addInputChannel(Channel(reader.get(), "three", "Video"));
        controller->addInputChannel(Channel(reader.get(), "four", "Video"));

        assertTrue(controller->openAndInit("SandBox"));
        assertTrue(controller->injectProcessingStateChange(ProcessingState::kRun));

        reader->addOutputChannel(Channel(reader.get(), "one", "Video", controller.get(), 2));

        assertTrue(reader->openAndInit("Video", testInputPath, true));
        assertTrue(reader->start());

        ACE_Reactor::instance()->run_reactor_event_loop();
    }

    // Now that SandBox has processed the input data, read in the output data to see if it is sane.
    //
    {
        FileReader::Ref reader(new FileReader);
        ACE_FILE_Addr outputAddr(testOutputPath);
        ACE_FILE_Connector outputConnector(reader->getDevice(), outputAddr);
        assertTrue(reader->fetchInput());
        assertTrue(reader->isMessageAvailable());
        {
            Decoder decoder(reader->getMessage());
            Video::Ref msg(decoder.decode<Video>());
            assertEqual(0.0, msg->getAzimuthStart());
            assertEqual(size_t(3), msg->size());
            Video::const_iterator pos = msg->begin();
            assertEqual(1, *pos++);
            assertEqual(2, *pos++);
            assertEqual(3, *pos++);
            assertTrue(pos == msg->end());

            assertTrue(reader->fetchInput());
            assertTrue(reader->isMessageAvailable());
        }
        {
            Decoder decoder(reader->getMessage());
            Video::Ref msg(decoder.decode<Video>());
            assertEqual(size_t(5), msg->size());
        }
    }

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

int
main(int argc, char** argv)
{
    return Test().mainRun();
}
