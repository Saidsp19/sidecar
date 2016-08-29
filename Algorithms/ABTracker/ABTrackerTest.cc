#include "ace/Event_Handler.h"
#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "Logger/Log.h"
#include "Messages/Extraction.h"
#include "Messages/TSPI.h"
#include "UnitTest/UnitTest.h"
#include "Utils/Utils.h"

#include "ABTracker.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Input {
    double when, range, azimuth, elevation;
};

Input inputs[] = {
    { 0.0, 100.0,  0.0, 0.0 },	// Track 1
    { 0.0, 100.0, 90.0, 0.0 },	// Track 2
    { 1.0, 110.0,  0.0, 0.0 },	// Track 1
    { 1.0, 110.0, 90.0, 0.0 },	// Track 2
    { 2.0, 120.0,  0.0, 0.0 },	// Track 1
    { 4.0, 140.0,  5.0, 0.0 },	// Track 1
    { 5.0, 150.0, 10.0, 0.0 },	// Track 1
    { 6.0, 160.0, 15.0, 0.0 },	// Track 1
    { 7.0, 170.0, 15.0, 0.0 },	// Track 1
    { 8.0, 180.0, 15.0, 0.0 },	// Track 1
    { 9.0, 190.0, 15.0, 0.0 },	// Track 1
    { 10.0, 200.0, 15.0, 0.0 },	// Track 1
};

const size_t inputCount = sizeof(inputs) / sizeof(Input);

struct Output {
    std::string tag;
    double when, range, azimuth, elevation;
    bool dropping;
};

Output outputs[] = {
    // With an initiation count of 2, we don't get the first report. { "1", 0.0, 100.0, 0.0, 0.0, false }, //
    // Track 1 { "2", 0.0, 100.0, 90.0, 0.0, false }, // Track 2
    { "1",  1.0, 110.0,  0.0, 0.0, false }, // Track 1
    { "2",  1.0, 110.0, 90.0, 0.0, false }, // Track 2
    { "1",  2.0, 120.0,  0.0, 0.0, false }, // Track 1
    { "1",  4.0, 140.0,  5.0, 0.0, false }, // Track 1
    { "2",  1.0, 110.0, 90.0, 0.0,  true }, // Track 2 drop
    { "1",  5.0, 150.0, 10.0, 0.0, false }, // Track 1
    { "1",  6.0, 160.0, 15.0, 0.0, false }, // Track 1
    { "1",  7.0, 170.0, 15.0, 0.0, false }, // Track 1
    { "1",  8.0, 180.0, 15.0, 0.0, false }, // Track 1
    { "1",  9.0, 190.0, 15.0, 0.0, false }, // Track 1
    { "1", 10.0, 200.0, 15.0, 0.0, false }, // Track 1
};

const size_t outputCount = sizeof(outputs) / sizeof(Output);

struct Test : public UnitTest::TestObj, public ACE_Event_Handler
{
    static Logger::Log& Log();

    Test() : UnitTest::TestObj("ABTracker"), ACE_Event_Handler(),
	     controller_(), iteration_(0) {}

    void test();

    void generateInput();

    void testOutput(size_t counter, const TSPI::Ref& output);

    int handle_timeout(const ACE_Time_Value&, const void*)
	{ ACE_Reactor::instance()->end_reactor_event_loop(); return 0; }

    Controller::Ref controller_;
    int iteration_;
};

struct WatchdogTimer : public ACE_Event_Handler
{
    WatchdogTimer();
    int handle_timeout(const ACE_Time_Value&, const void*)
	{ reactor()->end_reactor_event_loop(); return 0; }
};

WatchdogTimer::WatchdogTimer()
    : ACE_Event_Handler(ACE_Reactor::instance())
{
    const ACE_Time_Value delay(5);
    reactor()->schedule_timer(this, 0, delay);
}

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

    Sink() : Task(true) {}

    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    std::vector<TSPI::Ref> msgs_;
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

    MessageManager mgr(data);
    if (mgr.hasNative()) {
	LOGERROR << "metaType: " << mgr.getNativeMessageType() << std::endl;
	if (mgr.getNativeMessageType() == MetaTypeInfo::Value::kTSPI) {
	    TSPI::Ref msg(mgr.getNative<TSPI>());
	    msgs_.push_back(msg);
	    if (msgs_.size() == outputCount)
		ACE_Reactor::instance()->end_reactor_event_loop();
	}
    }

    return true;
}

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kInfo);

    Stream::Ref stream(Stream::Make("test"));

    // Create a task that will look at the output of the algorithm
    //
    TModule<Sink>* ctm = new TModule<Sink>(stream);
    assertEqual(0, stream->push(ctm));
    Sink::Ref sink = ctm->getTask();
    sink->setTaskName("Sink");
    sink->setTaskIndex(1);

    // Create the algorithm
    //
    ControllerModule* controllerMod = new ControllerModule(stream);
    assertEqual(0, stream->push(controllerMod));
    controller_ = controllerMod->getTask();
    controller_->setTaskIndex(0);

    sink->addInputChannel(Channel("input", "TSPI"));

    // Create three inputs and one output. Be sure that the output will properly connect to the Sink above.
    //
    controller_->addOutputChannel(Channel("output", "TSPI", sink));
    controller_->addInputChannel(Channel("input", "Extractions"));

    assertTrue(controller_->openAndInit("ABTracker"));
    assertTrue(controller_->injectProcessingStateChange(ProcessingState::kRun));
    ABTracker* alg = dynamic_cast<ABTracker*>(controller_->getAlgorithm());

    alg->setRotationDuration(1.0);
    alg->setTimeScaling(1.0);
    alg->setInitiationCount(2);
    alg->setAssociationRadius(50.0);
    alg->setCoastRotationCount(3.0);
    alg->setAlpha(1.0);
    alg->setBeta(0.0);

    generateInput();

    // Now run.
    //
    new WatchdogTimer;
    ACE_Reactor::instance()->run_reactor_event_loop();

    for (size_t index = 0; index < sink->msgs_.size(); ++index) {
	testOutput(index, sink->msgs_[index]);
    }

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

void
Test::generateInput()
{
    Logger::ProcLog log("generateInput", Log());
    LOGERROR << "iteration: " << iteration_ << std::endl;

    for (size_t index = 0; index < inputCount; ++index) {
	const Input& input(inputs[index]);
	Extractions::Ref msg(Extractions::Make("test", Header::Ref()));
	Extraction extraction(input.when, input.range,
                              Utils::degreesToRadians(input.azimuth),
                              input.elevation);
	msg->push_back(extraction);
	assertTrue(controller_->putInChannel(msg, 0));
    }
}

void
Test::testOutput(size_t counter, const TSPI::Ref& msg)
{
    Logger::ProcLog log("testOutput", Log());
    LOGERROR << "counter: " << counter << std::endl;
    LOGERROR << msg->dataPrinter() << std::endl;

    assertTrue(counter < outputCount);
    const Output& output(outputs[counter]);
    assertEqual(output.tag, msg->getTag());
    assertEqualEpsilon(output.when, msg->getWhen(), 0.0001);
    assertEqualEpsilon(output.range, msg->getRange() / 1000.0, 0.0001);
    assertEqualEpsilon(output.azimuth,
                       Utils::radiansToDegrees(msg->getAzimuth()),
                       0.0001);
    assertEqualEpsilon(output.elevation, msg->getElevation(), 0.0001);
    assertEqual(output.dropping, msg->isDropping());
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
