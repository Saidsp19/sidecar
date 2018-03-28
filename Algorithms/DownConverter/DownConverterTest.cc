#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"
#include "ace/Stream.h"

#include "Algorithms/ShutdownMonitor.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/Module.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/Readers.h"
#include "IO/ShutdownRequest.h"

#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "DownConverter.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("DownConverter") {}
    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    // Create a location for data output from the algorithm
    //
    Utils::TemporaryFilePath testOutputPath("DownConverterTestOutput");
    {
        ACE_Stream<ACE_MT_SYNCH> stream;

        // Add a shutdown monitor to the end of the processing stream.
        //
        assertEqual(0, stream.push(new TModule<ShutdownMonitor>()));

        // Add a writer that will write algorithm output data to a file. This one expects Video messages; for
        // another type, change the first open parameter.
        //
        TModule<FileWriterTask>* writer = new TModule<FileWriterTask>();
        assertEqual(0, stream.push(writer));
        assertTrue(writer->getTask()->openAndInit("Video", testOutputPath));

        // Create an algorithm controller and have it load our algorithm
        //
        TModule<Controller>* controller = new TModule<Controller>();
        assertEqual(0, stream.push(controller));
        assertTrue(controller->getTask()->openAndInit("DownConverter"));

        // Make sure the tasks are in the 'run' state.
        //
        stream.put(ProcessingStateChangeRequest(ProcessingState::kRun).getWrapped());

        // Access the running algorithm. Use the object pointer to change any runtime parameters (not shown
        // below)
        //
        DownConverter* algorithm = dynamic_cast<DownConverter*>(controller->getTask()->getAlgorithm());
        assertTrue(algorithm);

        // Create new Video message, initialized with some bogus data, and submit it to the stream (and thus our
        // algorithm). Create additional messages if necessary to properly test the algorithm.
        //
        VMEDataMessage vme;
        vme.header.azimuth = 0;
        int16_t init[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        Video::Ref msg(Video::Make("test", vme, init, init + 10));
        MessageManager mgr(msg);
        stream.put(mgr.getMessage(), 0);
        assertFalse(mgr.hasEncoded());

        // Send a shutdown request down the stream. This will cause the ShutdownMonitor task at the end of the
        // stream to stop the ACE event loop.
        //
        stream.put(ShutdownRequest().getWrapped());

        // Run the ACE event loop until stopped by the ShutdownMonitor
        //
        ACE_Reactor::instance()->run_reactor_event_loop();

        // Just to be safe, manually close the file writer task.
        //
        writer->getTask()->close(1);
    }

    // There should be data in the testOutputPath file. Create a reader to read the data in from the file, and
    // check the message for expected values.
    //
    FileReader::Ref reader(new FileReader);
    ACE_FILE_Addr inputAddr(testOutputPath);
    ACE_FILE_Connector inputConnector(reader->getDevice(), inputAddr);
    assertTrue(reader->fetchInput());
    assertTrue(reader->isMessageAvailable());
    Decoder decoder(reader->getMessage());
    {
        // Change the following checks so that they apply to the DownConverter algorithm.
        //
        Video::Ref msg(decoder.decode<Video>());
        assertEqual(size_t(10), msg->size());
        Video::const_iterator pos = msg->begin();
        assertEqual(101, *pos++);
        assertEqual(102, *pos++);
        assertEqual(103, *pos++);
        assertEqual(104, *pos++);
        assertEqual(105, *pos++);
        assertEqual(106, *pos++);
        assertEqual(107, *pos++);
        assertEqual(108, *pos++);
        assertEqual(109, *pos++);
        assertEqual(110, *pos++);
        assertTrue(pos == msg->end());
    }

    // Uncomment the following to always fail the test and see the log results. assertTrue(false);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
