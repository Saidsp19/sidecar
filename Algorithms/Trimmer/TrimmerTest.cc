#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"
#include "ace/Stream.h"

#include "Algorithms/ShutdownMonitor.h"
#include "IO/Readers.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "IO/Module.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/ShutdownRequest.h"
#include "IO/Stream.h"

#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "Trimmer.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    enum { kIterationLimit = 8 };

    static Logger::Log& Log();

    Test() : UnitTest::TestObj("Trimmer"), controller_(), iteration_(0) {}

    void test();

    void generateInput();

    void testOutput(size_t counter, const Video::Ref& output);

    Controller::Ref controller_;
    int iteration_;
};

Logger::Log&
Test::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("Test");
    return log_;
}

struct Sink : public Task
{
    using Ref = boost::shared_ptr<Sink>;

    static Logger::Log& Log();

    static Ref Make() { Ref ref(new Sink); return ref; }

    Sink() : Task(true), counter_(0), test_(0) {}

    void setTest(Test* test) { test_ = test; }

    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    int counter_;
    Test* test_;
};

Logger::Log&
Sink::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("Sink");
    return log_;
}

bool
Sink::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    std::clog << "deliverDataMessage - " << this << std::endl;

    Logger::ProcLog log("deliverDataMessage", Log());
    LOGERROR << "data: " << data << " test: " << test_ << std::endl;

    MessageManager mgr(data);
    LOGERROR << "count: " << counter_ << " message type: "
	     << mgr.getMessageType() << std::endl;

    if (mgr.hasNative()) {
	LOGERROR << "metaType: " << mgr.getNativeMessageType() << std::endl;
	if (mgr.getNativeMessageType() == MetaTypeInfo::Value::kVideo) {
	    Video::Ref msg(mgr.getNative<Video>());
	    LOGERROR << msg->dataPrinter() << std::endl;
	    test_->testOutput(counter_++, msg);
	}
    }

    return true;
}

void
Test::test()
{
    // Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    Stream::Ref stream(Stream::Make("test"));

    // Create a task that will look at the output of the algorithm
    //
    TModule<Sink>* ctm = new TModule<Sink>(stream);
    assertEqual(0, stream->push(ctm));
    Sink::Ref sink = ctm->getTask();
    sink->setTaskName("Sink");
    sink->setTest(this);
    sink->setTaskIndex(1);

    // Create the algorithm
    //
    ControllerModule* controllerMod = new ControllerModule(stream);
    assertEqual(0, stream->push(controllerMod));
    controller_ = controllerMod->getTask();
    controller_->setTaskIndex(0);

    sink->addInputChannel(Channel("input", "Video"));

    // Create three inputs and one output. Be sure that the output will properly connect to the Sink above.
    //
    controller_->addOutputChannel(Channel("output", "Video", sink));
    controller_->addInputChannel(Channel("one", "Video"));

    assertTrue(controller_->openAndInit("Trimmer"));
    assertTrue(controller_->injectProcessingStateChange(ProcessingState::kRun));

    generateInput();

    // Now run.
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

struct TestData {
    enum { kNumSamples = 8 };
    int firstSample;
    int maxSampleCount;
    bool complexSamples;
    int16_t input[8];
    int16_t output[8];
};

TestData data[Test::kIterationLimit] = {
    {
	0,
	0,
	false,
	{ 1, 2, 3, 4, 5, 6, 7, 8 },
	{ 1, 2, 3, 4, 5, 6, 7, 8 },
    },
    {
	0,
	0,
	true,
	{ 1, 2, 3, 4, 5, 6, 7, 8 },
	{ 1, 2, 3, 4, 5, 6, 7, 8 },
    },
    {
	0,
	1,
	false,
	{ 1, 2, 3, 4, 5, 6, 7, 8 },
	{ 1 },
    },
    {
	0,
	1,
	true,
	{ 1, 2, 3, 4, 5, 6, 7, 8 },
	{ 1, 2 },
    },
    {
	1,
	1,
	false,
	{ 1, 2, 3, 4, 5, 6, 7, 8 },
	{ 2 },
    },
    {
	1,
	1,
	true,
	{ 1, 2, 3, 4, 5, 6, 7, 8 },
	{ 3, 4 },
    },
    {
	1,
	2,
	false,
	{ 1, 2, 3, 4, 5, 6, 7, 8 },
	{ 2, 3},
    },
    {
	1,
	2,
	true,
	{ 1, 2, 3, 4, 5, 6, 7, 8 },
	{ 3, 4, 5, 6 },
    },
};

void
Test::generateInput()
{
    Logger::ProcLog log("generateInput", Log());
    LOGERROR << "iteration: " << iteration_ << std::endl;

    // Send data to the algorithm
    //
    VMEDataMessage vme;
    vme.header.azimuth = 0;

    const TestData& testData(data[iteration_++]);

    Trimmer* alg = dynamic_cast<Trimmer*>(controller_->getAlgorithm());
    assertTrue(alg);
    alg->setFirstSample(testData.firstSample);
    alg->setMaxSampleCount(testData.maxSampleCount);
    alg->setComplexSamples(testData.complexSamples);

    Video::Ref msg(Video::Make("test", vme,
                               testData.input,
                               testData.input + TestData::kNumSamples));
    LOGERROR << msg->dataPrinter() << std::endl;
    assertTrue(controller_->putInChannel(msg, 0));
}

void
Test::testOutput(size_t counter, const Video::Ref& msg)
{
    Logger::ProcLog log("testOutput", Log());
    LOGERROR << "counter: " << counter << std::endl;
    LOGERROR << msg->dataPrinter() << std::endl;

    const TestData& testData(data[counter]);
    size_t expectedSize = testData.maxSampleCount;
    if (testData.complexSamples)
	expectedSize *= 2;
    if (expectedSize == 0)
	expectedSize = TestData::kNumSamples;

    assertEqual(expectedSize, msg->size());

    for (size_t index = 0; index < msg->size(); ++index) {
	LOGERROR << index << " expected: " << int(testData.output[index])
		 << std::endl;
	assertEqual(int(testData.output[index]), int(msg[index]));
    }

    LOGERROR << "iteration: " << iteration_ << std::endl;

    if (iteration_ == kIterationLimit) {
	ACE_Reactor::instance()->end_reactor_event_loop();
    }
    else {
	generateInput();
    }
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
