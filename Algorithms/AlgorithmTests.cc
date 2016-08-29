#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"

#include "IO/Encoder.h"
#include "IO/FileReaderTask.h"
#include "IO/MessageManager.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/Stream.h"
#include "IO/Writers.h"
#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "Algorithm.h"
#include "Controller.h"
#include "ShutdownMonitor.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;
using namespace SideCar::Parameter;

struct AlgorithmTest : public UnitTest::TestObj
{
    AlgorithmTest() : TestObj("Algorithm") {}
    void test();
};

static int processedCount_ = 0;

class Blah : public Algorithm
{
public:
    Blah(Controller& controller, Logger::Log& log)
        : Algorithm(controller, log),
	  foo_(IntValue::Make("foo", "foo test", 1)),
	  bar_(DoubleValue::Make("bar", "bar test", 0.0)) {}

    bool startup()
        {
            getLog().info() << "STARTUP" << std::endl;
            registerProcessor<Blah,Video>("A", &Blah::process);
            registerParameter(foo_);
            registerParameter(bar_);
            processedCount_ = 0;
            return Algorithm::startup();
        }

    bool reset()
        {
            getLog().info() << "RESET" << std::endl;
            processedCount_ = 0;
            return Algorithm::reset();
        }

    /** Implementation of the Algorithm::process interface.

        \param decoder CDR object containing the raw data to process

        \param inputKey identifier of the device that provided the data

        \return true if successful, false otherwise
    */
    bool process(const Video::Ref& msg);

    bool stop()
        {
            getLog().info() << "STOP" << std::endl;
            ACE_Reactor::instance()->end_event_loop();
            return Algorithm::stop();
        }

    int getProcessedCount() const { return processedCount_; }

    IntValue::Ref foo_;
    DoubleValue::Ref bar_;
};

bool
Blah::process(const Video::Ref& msg)
{
    getLog().info() << "PROCESS - " << ++processedCount_ << std::endl;
    return send(msg);
}

void
AlgorithmTest::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug1);

    Utils::TemporaryFilePath fp("algorithmTestsOutput");
    ACE_FILE_Addr addr(fp.getFilePath().c_str());
    IO::FileWriter::Ref writer(IO::FileWriter::Make());
    ACE_FILE_Connector fd(writer->getDevice(), addr);

    VMEDataMessage vme;
    Video::Ref msg(Video::Make("hi", vme, 3));
    msg->push_back(100);
    msg->push_back(200);
    msg->push_back(300);
    for (int count = 0;  count < 10; ++count){
        IO::MessageManager mgr(msg);
        assertTrue(writer->write(mgr.getMessage()));
    }

    IO::Stream::Ref stream(IO::Stream::Make("AlgorithmTest"));
    stream->push(new ShutdownMonitorModule(stream));

    ControllerModule* module = new ControllerModule(stream);
    assertEqual(0, stream->push(module));

    Controller* controller = module->getTask().get();
    controller->setTaskIndex(1);
    Blah* blah = new Blah(*controller, Logger::Log::Root());

    IO::FileReaderTaskModule* reader = new IO::FileReaderTaskModule(stream) ;
    assertEqual(0, stream->push(reader));

    IO::Channel output("A", "Video", module->getTask());
    reader->getTask()->addOutputChannel(output);

    controller->addOutputChannel(IO::Channel("B", "Video"));
    controller->addInputChannel(IO::Channel("A", "Video"));

    assertTrue(controller->openAndInit("blah", "", blah));
    assertTrue(controller->injectProcessingStateChange(IO::ProcessingState::kAutoDiagnostic));

    assertTrue(reader->getTask()->openAndInit("Video", fp.filePath(), true));
    assertTrue(reader->getTask()->start());

    ACE_Reactor::instance()->run_reactor_event_loop();

    stream->close();

    assertEqual(10, processedCount_);
}

int
main(int argc, char** argv)
{
    return (new AlgorithmTest)->mainRun();
}
