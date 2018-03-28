#include "ace/Reactor.h"

#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "IO/Task.h"

#include "Algorithms/Controller.h"
#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"
#include "UnitTest/UnitTest.h"

#include "EdgeDetector.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct TestData {
    enum { kNumSamples = 8 };
    EdgeDetector::Edge operation;
    int16_t input[kNumSamples];
    bool output[kNumSamples];
};

TestData data[] = {
    {EdgeDetector::kLeading, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}},
    {EdgeDetector::kLeading, {0, 1, 1, 2, 3, 3, 2, 0}, {0, 1, 0, 1, 1, 0, 0, 0}},
    {EdgeDetector::kLeading, {0, 1, 2, 3, 1, 2, 3, 0}, {0, 1, 1, 0, 0, 1, 0, 0}},
    {EdgeDetector::kLeading, {0, 1, 2, 3, 2, 1, 2, 0}, {0, 1, 1, 0, 0, 0, 0, 0}},
    {EdgeDetector::kLeading, {0, 3, 2, 1, 3, 2, 1, 0}, {0, 0, 0, 0, 0, 0, 0, 0}},
    {EdgeDetector::kLeading, {0, 3, 2, 1, 2, 3, 2, 0}, {0, 0, 0, 0, 1, 0, 0, 0}},

    {EdgeDetector::kTrailing, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}},
    {EdgeDetector::kTrailing, {0, 1, 1, 2, 3, 3, 2, 0}, {0, 0, 0, 0, 0, 1, 1, 0}},
    {EdgeDetector::kTrailing, {0, 1, 2, 3, 1, 2, 3, 0}, {0, 0, 0, 0, 0, 0, 0, 0}},
    {EdgeDetector::kTrailing, {0, 1, 2, 3, 2, 1, 2, 0}, {0, 0, 0, 0, 1, 0, 0, 0}},
    {EdgeDetector::kTrailing, {4, 3, 2, 1, 3, 2, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}},
};

struct Test : public UnitTest::TestObj {
    enum { kIterationLimit = sizeof(data) / sizeof(TestData) };

    static Logger::Log& Log();

    Test() : UnitTest::TestObj("EdgeDetector"), controller_(), iteration_(0) {}

    void test();

    void generateInput();

    void testOutput(size_t counter, const BinaryVideo::Ref& output);

    Controller::Ref controller_;
    int iteration_;
};

Logger::Log&
Test::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("Test");
    return log_;
}

struct Sink : public Task {
    using Ref = boost::shared_ptr<Sink>;

    static Logger::Log& Log();

    static Ref Make()
    {
        Ref ref(new Sink);
        return ref;
    }

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
    LOGERROR << "count: " << counter_ << " message type: " << mgr.getMessageType() << std::endl;

    if (mgr.hasNative()) {
        LOGERROR << "metaType: " << mgr.getNativeMessageType() << std::endl;
        if (mgr.getNativeMessageType() == MetaTypeInfo::Value::kBinaryVideo) {
            BinaryVideo::Ref msg(mgr.getNative<BinaryVideo>());
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

    sink->addInputChannel(Channel("input", "BinaryVideo"));

    // Create three inputs and one output. Be sure that the output will properly connect to the Sink above.
    //
    controller_->addOutputChannel(Channel("output", "BinaryVideo", sink));
    controller_->addInputChannel(Channel("one", "Video"));

    assertTrue(controller_->openAndInit("EdgeDetector"));
    assertTrue(controller_->injectProcessingStateChange(ProcessingState::kRun));

    generateInput();

    // Now run.
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

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

    EdgeDetector* alg = dynamic_cast<EdgeDetector*>(controller_->getAlgorithm());
    assertTrue(alg);
    alg->setDetectionType(EdgeDetector::Edge(testData.operation));

    Video::Ref msg(Video::Make("test", vme, testData.input, testData.input + TestData::kNumSamples));
    LOGERROR << msg->dataPrinter() << std::endl;
    assertTrue(controller_->putInChannel(msg, 0));
}

void
Test::testOutput(size_t counter, const BinaryVideo::Ref& msg)
{
    Logger::ProcLog log("testOutput", Log());
    LOGERROR << "counter: " << counter << std::endl;
    LOGERROR << msg->dataPrinter() << std::endl;

    const TestData& testData(data[counter]);
    assertEqual(size_t(TestData::kNumSamples), msg->size());

    for (size_t index = 0; index < msg->size(); ++index) {
        LOGERROR << index << " expected: " << int(testData.output[index]) << std::endl;
        assertEqual(testData.output[index], bool(msg[index]));
    }

    LOGERROR << "iteration: " << iteration_ << std::endl;

    if (iteration_ == kIterationLimit) {
        ACE_Reactor::instance()->end_reactor_event_loop();
    } else {
        generateInput();
    }
}

int
main(int argc, char** argv)
{
    return Test().mainRun();
}
