#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Time/TimeStamp.h"
#include "UnitTest/UnitTest.h"

#include "MatchedFilter2.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

/** Test data container for MatchedFilter testing.
 */
struct TestData {
    enum {
        kNumInputs = 2,                      // Number of inputs
        kNumSamples = 8,                     // Number of samples in the test data
        kNumCopies = 1000,                   // Number of copies to make of the sample data
        kOutSize = kNumSamples * kNumCopies, // Expected size of output msg
        kNumIterations = 800                 // Number times to use
    };

    int16_t inputs[kNumInputs][kNumSamples];
    int16_t output[kNumSamples];
};

TestData data[] = {
    {
        {
            {1000, -100, 100, -10, 10, -100, 100, -100}, // transmit
            {800, -80, 80, -8, 8, -80, 80, -80},         // receive
        },
        {803, 0, 80, 0, 15, -78, 87, -71} // expected output
    },

    // !!! ADD MORE TESTS !!!
    //
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
    Test() : UnitTest::TestObj("MatchedFilter"), controller_(), iteration_(0) {}

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
        if (mgr.getNativeMessageType() == MetaTypeInfo::kVideo) {
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
    sink->addInputChannel(Channel(controller_, "input", "Video"));
    controller_->addOutputChannel(Channel(controller_, "output", "Video", sink, 0));

    // Create the inputs channels for the algorithm.
    //
    controller_->addInputChannel(Channel(Task::Ref(), "transmit", "Video"));
    controller_->addInputChannel(Channel(Task::Ref(), "receive", "Video"));

    // Initialize everything and put into the processing state.
    //
    assertTrue(controller_->openAndInit("MatchedFilter"));
    assertTrue(controller_->injectProcessingStateChange(ProcessingState::kRun));

    // Update the algorithm with the settings appropriate for this message set.
    //
    MatchedFilter* alg = dynamic_cast<MatchedFilter*>(controller_->getAlgorithm());
    assertTrue(alg);

    // Configure the matched filter
    //
    alg->setFFTSize(1024);
    alg->setTxPulseStartBin(0);
    alg->setTxPulseSpan(1);
    alg->setTxThreshold(700);
    alg->setTxThresholdStartBin(0);
    alg->setTxThresholdSpan(1);
    alg->setRxFilterStartBin(0);
    alg->setRxFilterSpan(0);

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
main(int argc, char** argv)
{
    return Test().mainRun();
}
