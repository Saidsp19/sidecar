#include <cstdio>
#include <fstream>
#include <iostream>

#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"

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

#include "Filter.h"

#ifdef linux
#include <linux/version.h>
#endif

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;
using namespace SideCar::Parameter;

inline std::ostream&
operator<<(std::ostream& os, const Video::Ref& ref)
{
    return ref->print(os);
}

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("Filter") {}
    void test();
};

void
Test::test()
{
    Logger::Log& log = Logger::Log::Root();
    // log.setPriorityLimit(Logger::Priority::kDebug);
    Utils::TemporaryFilePath testOutputPath("filterTestOutput");
    Utils::TemporaryFilePath filterTestKernel("filterTestKernel.csv");
    {
        Stream::Ref stream(Stream::Make("test"));

        assertEqual(0, stream->push(new ShutdownMonitorModule(stream)));

        FileWriterTaskModule* writer = new FileWriterTaskModule(stream);
        assertEqual(0, stream->push(writer));
        assertTrue(writer->getTask()->openAndInit("Video", testOutputPath));

        ControllerModule* controller = new ControllerModule(stream);
        assertEqual(0, stream->push(controller));
        assertTrue(controller->getTask()->openAndInit("Filter"));
        stream->put(ProcessingStateChangeRequest(ProcessingState::kRun).getWrapped());

        // Write out a test filter
        {
            ofstream f(filterTestKernel, std::ios_base::out | std::ios_base::trunc);
            f << "3, 3," << std::endl
              << " 0, 0, 0," << std::endl
              << " 0, 1, 0," << std::endl
              << " 0, 0, 0," << std::endl;
            f.close();
        }

        Filter* filter = dynamic_cast<Filter*>(controller->getTask()->getAlgorithm());
        assertTrue(filter);

        filter->setKernelFilePath(filterTestKernel);

        int16_t init[] = {0, 1, 0, 0, 2, 0, 0, 3, 3, 0, 0, 2, 0, 0, 3, 0, 0, 4, 4, 0,
                          0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 3, 3, 3, 3};
        int16_t init2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

        VMEDataMessage vme;
        vme.header.azimuth = 0;
        {
            Video::Ref msg(Video::Make("test", vme, init, init + 10));
            MessageManager mgr(msg);
            stream->put(mgr.getMessage(), 0);
        }
        {
            Video::Ref msg(Video::Make("test", vme, init + 10, init + 20));
            MessageManager mgr(msg);
            stream->put(mgr.getMessage(), 0);
        }
        {
            Video::Ref msg(Video::Make("test", vme, init + 20, init + 30));
            MessageManager mgr(msg);
            stream->put(mgr.getMessage(), 0);
        }
        {
            Video::Ref msg(Video::Make("test", vme, init + 30, init + 40));
            MessageManager mgr(msg);
            stream->put(mgr.getMessage(), 0);
        }
        {
            Video::Ref msg(Video::Make("test", vme, init2, init2 + 15));
            MessageManager mgr(msg);
            stream->put(mgr.getMessage(), 0);
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
        LOGERROR << "first: " << msg << std::endl;
        assertEqual(0, msg[0]);
        assertEqual(0, msg[1]);
        assertEqual(0, msg[2]);
        assertEqual(0, msg[3]);
        assertEqual(0, msg[4]);
        assertEqual(0, msg[5]);
        assertEqual(0, msg[6]);
        assertEqual(0, msg[7]);
        assertEqual(0, msg[8]);
        assertEqual(0, msg[9]);
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
        Decoder decoder(reader->getMessage());
        Video::Ref msg(decoder.decode<Video>());
        LOGERROR << "second: " << msg << std::endl;
        assertEqual(0, msg[0]);
        assertEqual(0, msg[1]);
        assertEqual(0, msg[2]);
        assertEqual(0, msg[3]);
        assertEqual(0, msg[4]);
        assertEqual(0, msg[5]);
        assertEqual(0, msg[6]);
        assertEqual(0, msg[7]);
        assertEqual(0, msg[8]);
        assertEqual(0, msg[9]);
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
        Decoder decoder(reader->getMessage());
        Video::Ref msg(decoder.decode<Video>());
        LOGERROR << "third: " << msg << std::endl;
        assertEqual(0, msg[0]);
        assertEqual(1, msg[1]);
        assertEqual(0, msg[2]);
        assertEqual(0, msg[3]);
        assertEqual(2, msg[4]);
        assertEqual(0, msg[5]);
        assertEqual(0, msg[6]);
        assertEqual(0, msg[7]);
        assertEqual(0, msg[8]);
        assertEqual(0, msg[9]);
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
        Decoder decoder(reader->getMessage());
        Video::Ref msg(decoder.decode<Video>());
        LOGERROR << "fourth: " << msg << std::endl;
        assertEqual(0, msg[0]);
        assertEqual(1, msg[1]);
        assertEqual(0, msg[2]);
        assertEqual(0, msg[3]);
        assertEqual(2, msg[4]);
        assertEqual(0, msg[5]);
        assertEqual(0, msg[6]);
        assertEqual(0, msg[7]);
        assertEqual(0, msg[8]);
        assertEqual(0, msg[9]);
    }

    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    {
        Decoder decoder(reader->getMessage());
        Video::Ref msg(decoder.decode<Video>());
        LOGERROR << "fifth: " << msg << std::endl;
        assertEqual(0, msg[0]);

        // Hmmm. Why the discrepency between the two? Rounding modes?
        //
#ifdef linux
        assertEqual(0, msg[1]);
#else
        assertEqual(1, msg[1]);
#endif
        assertEqual(0, msg[2]);
        assertEqual(0, msg[3]);
#ifdef linux
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
        assertEqual(1, msg[4]);
#else
        assertEqual(2, msg[4]);
#endif
#else
        assertEqual(2, msg[4]);
#endif
        assertEqual(0, msg[5]);
        assertEqual(0, msg[6]);
        assertEqual(0, msg[7]);
        assertEqual(0, msg[8]);
        assertEqual(0, msg[9]);
        assertEqual(0, msg[10]);
        assertEqual(0, msg[11]);
        assertEqual(0, msg[12]);
        assertEqual(0, msg[13]);
        assertEqual(0, msg[14]);
    }

    assertFalse(reader->fetchInput());
    // assertTrue(false);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
