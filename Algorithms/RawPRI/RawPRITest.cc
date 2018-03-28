#include <iostream>

#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"
#include "ace/Stream.h"

#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/Module.h"
#include "IO/Readers.h"

#include "Logger/Log.h"
#include "Messages/RawVideo.h"
#include "Messages/Video.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "RawPRI.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("RawPRI") {}
    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Utils::TemporaryFilePath testOutputPath("RawPRITestOutput");
    {
        ACE_Stream<ACE_MT_SYNCH> stream;

        Module<FileWriterTask>* writer = new Module<FileWriterTask>("Writer");
        assertEqual(0, stream.push(writer));
        assertTrue(writer->getTask()->openAndInit("Video", testOutputPath));

        Module<Controller>* controller = new Module<Controller>("RawPRI");
        assertEqual(0, stream.push(controller));
        assertTrue(controller->getTask()->openAndInit("RawPRI"));
        controller->getTask()->enterRunState();

        RawPRI* rawPRI = dynamic_cast<RawPRI*>(controller->getTask()->getAlgorithm());
        assertTrue(rawPRI);

        int16_t init[] = {0x0000, 0x0003, 0x00FC, 0x00FF, 0xFFFC, 0xFFFF, 0x7FFF};

        RawVideo::Ref msg(RawVideo::Make("test", 0.0015339808, 0, 4096, 0, 0.0, 300.0, 7, init, init + 7));
        std::cerr << *msg.get() << std::endl;
        assertEqual(size_t(7), msg->size());
        RawVideo::const_iterator pos = msg->begin();
        assertEqual(0, *pos++);
        assertEqual(0x0003, *pos++);
        assertEqual(0x00FC, *pos++);
        assertEqual(0x00FF, *pos++);
        assertEqual(int16_t(0xFFFC), *pos++);
        assertEqual(int16_t(0xFFFF), *pos++);
        assertEqual(int16_t(0x7FFF), *pos++);
        assertTrue(pos == msg->end());

        MessageManager mgr(msg);
        stream.put(mgr.getMessage(), 0);
        assertFalse(mgr.hasEncoded());
    }

    FileReader::Ref reader(new FileReader);
    ACE_FILE_Addr inputAddr(testOutputPath.c_str());
    ACE_FILE_Connector inputConnector(reader->getDevice(), inputAddr);
    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    Decoder decoder(reader->getMessage());
    {
        Video::Ref msg(decoder.decode<Video>());
        assertEqual(size_t(7), msg->size());
        Video::const_iterator pos = msg->begin();
        std::cerr << *msg.get() << std::endl;
        assertEqual(0, *pos++);
        assertEqual(0x0003, *pos++);
        assertEqual(0x00FC, *pos++);
        assertEqual(0x00FF, *pos++);
        assertEqual(int16_t(0xFFFC), *pos++);
        assertEqual(int16_t(0xFFFF), *pos++);
        assertEqual(int16_t(0x7FFF), *pos++);
        assertTrue(pos == msg->end());
    }

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
