#include "ace/Reactor.h"

#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "IO/Task.h"

#include "Algorithms/Controller.h"
#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"
#include "Time/TimeStamp.h"
#include "UnitTest/UnitTest.h"

#include "DynamicThreshold.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

/** Test data container for DyamicThreshold testing.
 */
struct TestData {
    enum {
        kNumInputs = 2,                      // Number of inputs
        kNumSamples = 8,                     // Number of samples in the test data
        kNumCopies = 1000,                   // Number of copies to make of the sample data
        kOutSize = kNumSamples * kNumCopies, // Expected size of output msg
        kNumIterations = 1000                // Number times to use
    };

    DynamicThreshold::Operator op; // Parameter setting for the algorithm

    int16_t inputs[kNumInputs][kNumSamples]; // Input data
    int16_t output[kNumSamples];             // Expected output data
};

TestData data[] = {
    {
        DynamicThreshold::kLessThan,
        {{0, 1, 2, 3, 4, 5, 6, 7},  // samples
         {2, 2, 2, 2, 2, 2, 2, 2}}, // thresholds
        {1, 1, 0, 0, 0, 0, 0, 0}    // output
    },
    {
        DynamicThreshold::kLessThanEqualTo,
        {{0, 1, 2, 3, 4, 5, 6, 7},  // samples
         {2, 2, 2, 2, 2, 2, 2, 2}}, // thresholds
        {1, 1, 1, 0, 0, 0, 0, 0}    // output
    },
    {
        DynamicThreshold::kEqualTo,
        {{0, 1, 2, 3, 4, 5, 6, 7},  // samples
         {2, 2, 2, 2, 2, 2, 2, 2}}, // thresholds
        {0, 0, 1, 0, 0, 0, 0, 0}    // output
    },
    {
        DynamicThreshold::kGreaterThanEqualTo,
        {{0, 1, 2, 3, 4, 5, 6, 7},  // samples
         {2, 2, 2, 2, 2, 2, 2, 2}}, // thresholds
        {0, 0, 1, 1, 1, 1, 1, 1}    // output
    },
    {
        DynamicThreshold::kGreaterThan,
        {{0, 1, 2, 3, 4, 5, 6, 7},  // samples
         {2, 2, 2, 2, 2, 2, 2, 2}}, // thresholds
        {0, 0, 0, 1, 1, 1, 1, 1}    // output
    },
};

/** TestObj derivative that performs the DynamicThreshold unit tests. The test() method creates a Stream with
    two Task objects, the first hosting the DynamicThreshold algorithm followed by a Sink object used to fetch
    the output of the algorithm. The Sink::deliverDataMessage() method, when it has a valid output message from
    the algorithm, will invoke our testOutput() method to compare the output sample values with the expected
    values defined above in the TestData container. After kIterationLimit messages, testOutput() signals ACE to
    stop its processing loop, which cleanly shuts down the Stream and its held Task objects. Otherwise, it
    invokes generateInput() to feed another set of messages to the DynamicThreshold algorithm.
*/
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
    Test() : UnitTest::TestObj("DynamicThreshold"), controller_(), iteration_(0) {}

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
    void testOutput(size_t counter, const BinaryVideo::Ref& output);

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

    // Use a MessageManager to obtain info about the given raw data.
    //
    MessageManager mgr(data);
    LOGDEBUG << "count: " << counter_ << " message type: " << mgr.getMessageType() << std::endl;

    if (mgr.hasNative()) {
        LOGDEBUG << "metaType: " << mgr.getNativeMessageType() << std::endl;
        if (mgr.getNativeMessageType() == MetaTypeInfo::Value::kBinaryVideo) {
            BinaryVideo::Ref msg(mgr.getNative<BinaryVideo>());
            test_->testOutput(counter_++, msg);
        }
    }

    return true;
}

void
Test::test()
{
    // Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

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
    sink->addInputChannel(Channel("input", "BinaryVideo"));
    controller_->addOutputChannel(Channel("output", "BinaryVideo", sink));

    // Create two inputs channels for the algorithm. The first will contain samples, the second threshold
    // values.
    //
    controller_->addInputChannel(Channel("samples", "Video"));
    controller_->addInputChannel(Channel("thresholds", "Video"));

    // Initialize everything and put into the processing state.
    //
    assertTrue(controller_->openAndInit("DynamicThreshold"));
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
    DynamicThreshold* alg = dynamic_cast<DynamicThreshold*>(controller_->getAlgorithm());
    assertTrue(alg);
    alg->setOperation(testData.op);

    // Generate a message for each input channel.
    //
    for (size_t input = 0; input < TestData::kNumInputs; ++input) {
        Video::Ref msg(Video::Make("test", vme, TestData::kOutSize));
        msg->setMessageSequenceNumber(iteration_);

        // Each message has a run of TestData::kNumCopies values that repeat TestData::kNumCopies in order to
        // simulate real-world message sizes.
        //
        for (size_t count = 0; count < TestData::kNumCopies; ++count) {
            std::copy(testData.inputs[input], testData.inputs[input] + TestData::kNumSamples,
                      std::back_inserter(msg->getData()));
        }
        LOGDEBUG << msg->dataPrinter() << std::endl;

        // Post the input message to the appropriate algorithm input channel
        //
        assertTrue(controller_->putInChannel(msg, input));
    }
}

void
Test::testOutput(size_t counter, const BinaryVideo::Ref& msg)
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
        LOGINFO << index << " expected: " << int(testData.output[index]) << std::endl;
        assertEqual(testData.output[index], bool(msg[index]));
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
main(int argc, char** argv)
{
    return Test().mainRun();
}
