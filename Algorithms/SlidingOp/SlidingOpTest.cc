#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Time/TimeStamp.h"
#include "UnitTest/UnitTest.h"

#include "SlidingOp.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

/** Test data container for SlidingOp testing.
 */
struct TestData {
    enum {
        kNumSamples = 12,                    // Number of samples in the test data
        kNumCopies = 1,                      // Number of copies to make of the sample data
        kOutSize = kNumSamples * kNumCopies, // Expected size of output msg
        kNumIterations = 1                   // Number times to use
    };

    SlidingOp::Operation op;
    int initialOffset;
    int windowSize;
    int emptyValue;
    int input[kNumSamples];
    int output[kNumSamples];
};

TestData data[] = {
    {
        SlidingOp::kSumOp,
        -1,
        3,
        0,
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
        {3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 23},
    },
    {
        SlidingOp::kProdOp,
        -1,
        3,
        0,
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
        {0, 6, 24, 60, 120, 210, 336, 504, 720, 990, 1320, 0},
    },
    {SlidingOp::kMinOp,
     -2,
     5,
     0,
     {
         1,
         2,
         3,
         4,
         5,
         4,
         3,
         2,
         1,
         2,
         3,
         4,
     },
     {
         0,
         0,
         1,
         2,
         3,
         2,
         1,
         1,
         1,
         1,
         0,
         0,
     }},
    {
        SlidingOp::kMaxOp,
        -1,
        3,
        0,
        {
            1,
            2,
            3,
            4,
            5,
            4,
            3,
            2,
            1,
            2,
            3,
            4,
        },
        {
            2,
            3,
            4,
            5,
            5,
            5,
            4,
            3,
            2,
            3,
            4,
            4,
        },
    },
    {
        SlidingOp::kAverageOp,
        -1,
        3,
        0,
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 8},
    },
    {
        SlidingOp::kAverageOp,
        -2,
        5,
        0,
        {1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3},
        {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    },
    {
        SlidingOp::kMedianOp,
        -1,
        3,
        0,
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 11},
    },
    {
        SlidingOp::kMedianOp,
        -1,
        3,
        12,
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
        {2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
    },
};

struct Test : public UnitTest::TestObj {
    enum {

        // Number of messages defined in the TestData container above.
        //
        kNumTestDefinitions = sizeof(data) / sizeof(TestData),
        kNumTests = kNumTestDefinitions * TestData::kNumIterations,
    };

    static Logger::Log& Log();

    /** Constructor.
     */
    Test() : UnitTest::TestObj("SlidingOp"), controller_(), iteration_(0) {}

    /** Implementation of TestObj interface. Creates the processing Stream object, its internal Task objects,
        properly initializes everything, and enters an ACE event loop, which does not return until testOutput()
        signals the event loop to exit.
    */
    void test();

    /** Generate a set of input messages for the algorithm to consume.
     */
    void generateInput();

    /** Compare the output from the algorithm with the sample values defined in the TestData container above.

        \param counter the message counter

        \param output the message output from the algorithm
    */
    void testOutput(size_t counter, const Video::Ref& output);

private:
    Controller::Ref controller_;
    int iteration_;
};

Logger::Log&
Test::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("Test");
    return log_;
}

/** Simple Task object that just implements the deliverDataMessage() method of the Task interface so that it
    knows when the algorithm ahead of it in the stream emitted an output message. For data messages,
    deliverDataMessage() invokes the Test::testOutput() method.
*/
struct Sink : public Task {
    using Ref = boost::shared_ptr<Sink>;

    static Logger::Log& Log();

    /** Factory method used by the TModule class to generate a new Sink object.

        \return new Sink object
    */
    static Ref Make()
    {
        Ref ref(new Sink);
        return ref;
    }

    /** Install a reference to the active Test object so that deliverDataMessage() may invoke its testOutput()
        method.

        \param test the Test object to use
    */
    void setTest(Test* test) { test_ = test; }

    /** Implementation of Task interface. Invoked when a data message is available for consumption by the task.

        \param data the ACE wrapper containing the message

        \param timeout ignored

        \return true if message was processed
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

private:
    Sink() : Task(true), counter_(0), test_(0) {}

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
    Logger::ProcLog log("deliverDataMessage", Log());
    LOGINFO << "data: " << data << " test: " << test_ << std::endl;

    MessageManager mgr(data);
    LOGDEBUG << "count: " << counter_ << " message type: " << mgr.getMessageType() << std::endl;

    if (mgr.hasNative()) {
        LOGDEBUG << "metaType: " << mgr.getNativeMessageType() << std::endl;
        if (mgr.getNativeMessageType() == MetaTypeInfo::Value::kVideo) {
            Video::Ref msg(mgr.getNative<Video>());
            LOGDEBUG << msg->dataPrinter() << std::endl;
            test_->testOutput(counter_++, msg);
        }
    }

    return true;
}

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Logger::Log::Find("SideCar.Algorithms").setPriorityLimit(Logger::Priority::kDebug);
    Logger::Log::Find("WindowGenerator").setPriorityLimit(Logger::Priority::kDebug);

    // Create a new data processing stream to host the algorithm and sink tasks.
    //
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

    // Connect the output of the algorithm to the input of the sink.
    //
    sink->addInputChannel(Channel("input", "Video"));
    controller_->addOutputChannel(Channel("output", "Video", sink));

    // Create the input channel for the algorithm.
    //
    controller_->addInputChannel(Channel("input", "Video"));

    // Initialize everything and put into the processing state.
    //
    assertTrue(controller_->openAndInit("SlidingOp"));
    assertTrue(controller_->injectProcessingStateChange(ProcessingState::kRun));

    // Generate our first input test messages.
    //
    generateInput();

    // Now run.
    //
    Time::TimeStamp begin(Time::TimeStamp::Now());
    ACE_Reactor::instance()->run_reactor_event_loop();
    Time::TimeStamp delta(Time::TimeStamp::Now());
    delta -= begin;

    std::clog << "duration: " << delta.asDouble() << " seconds" << std::endl;

    double timePerSample = delta.asDouble() / (kNumTests * TestData::kOutSize);
    std::clog << " time per sample: " << timePerSample << std::endl;

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

void
Test::generateInput()
{
    Logger::ProcLog log("generateInput", Log());
    LOGINFO << "iteration: " << iteration_ << std::endl;

    // Simulated VME settings. Be sure to adjust the azimuth if the algorithm under test depends on it.
    //
    VMEDataMessage vme;
    vme.header.azimuth = 0;

    // Obtain the test data to use for the input messages.
    //
    const TestData& testData(data[iteration_++ % kNumTestDefinitions]);

    // Update the algorithm with the settings appropriate for this message set.
    //
    SlidingOp* alg = dynamic_cast<SlidingOp*>(controller_->getAlgorithm());
    assertTrue(alg);

    alg->setOperation(testData.op);
    alg->setInitialOffset(testData.initialOffset);
    alg->setWindowSize(testData.windowSize);
    alg->setEmptyValue(testData.emptyValue);

    Video::Ref msg(Video::Make("test", vme, TestData::kOutSize));
    msg->setMessageSequenceNumber(iteration_);

    // Each message has a run of TestData::kNumCopies values that repeat TestData::kNumCopies in order to
    // simulate real-world message sizes.
    //
    for (size_t count = 0; count < TestData::kNumCopies; ++count) {
        std::copy(testData.input, testData.input + TestData::kNumSamples, std::back_inserter(msg->getData()));
    }

    LOGDEBUG << msg->dataPrinter() << std::endl;

    // Post the input message to the appropriate algorithm input channel
    //
    assertTrue(controller_->putInChannel(msg, 0));
}

void
Test::testOutput(size_t counter, const Video::Ref& msg)
{
    Logger::ProcLog log("testOutput", Log());
    LOGINFO << "counter: " << counter << std::endl;
    LOGDEBUG << msg->dataPrinter() << std::endl;

    // Verify that the message has the expected number of samples
    //
    const TestData& testData(data[counter % kNumTestDefinitions]);
    assertEqual(size_t(TestData::kOutSize), msg->size());

    // Iterate over the first run of TestData::kNumSamples samples and compare with the expected values defined
    // in the TestData container.
    //
    for (size_t index = 0; index < TestData::kNumSamples; ++index) {
        LOGDEBUG << index << " expected: " << int(testData.output[index]) << std::endl;
        assertEqual(int(testData.output[index]), int(msg[index]));
    }

    LOGDEBUG << "iteration: " << iteration_ << std::endl;

    // See if we are done with the test.
    //
    if (iteration_ == kNumTests) {
        ACE_Reactor::instance()->end_reactor_event_loop();
    } else {
        generateInput();
    }
}

int
main(int argc, const char** argv)
{
    return Test().mainRun();
}
