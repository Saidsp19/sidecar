#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "IO/Task.h"
#include "Logger/Log.h"
#include "Messages/Video.h"
#include "UnitTest/UnitTest.h"

#include "Clamp.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    static constexpr int kMaxMessageCount = 10;
    static constexpr int kMessageSize = 10;
    static constexpr int kMin = -1;
    static constexpr int kMax = 3;

    Test() : UnitTest::TestObj("Clamp"), messageCounter_(0) {}

    void test();

    bool testOutput(const Video::Ref& output);

    int messageCounter_;
};

struct Sink : public Task
{
    using Ref = boost::shared_ptr<Sink>;

    static Logger::Log& Log();

    static auto Make() { Ref ref(new Sink); return ref; }

    Sink() : Task(true), test_(0) {}

    void setTest(Test* test) { test_ = test; }
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

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
    LOGERROR << std::endl;

    MessageManager mgr(data);
    if (mgr.hasNative()) {
	LOGERROR << "metaType: " << mgr.getNativeMessageType() << std::endl;
	if (mgr.getNativeMessageType() == MetaTypeInfo::Value::kVideo) {
	    Video::Ref msg(mgr.getNative<Video>());
	    LOGERROR << msg->dataPrinter() << std::endl;
	    if (test_->testOutput(msg)) {
		LOGINFO << "shutting down server" << std::endl;
		ACE_Reactor::instance()->end_reactor_event_loop();
	    }
	}
    }

    return true;
}

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    Stream::Ref stream(Stream::Make("test"));

    // Create a task that will look at the output of the algorithm
    //
    TModule<Sink>* ctm = new TModule<Sink>(stream);
    assertEqual(0, stream->push(ctm));
    Sink::Ref sink = ctm->getTask();
    sink->setTaskIndex(1);
    sink->setTest(this);

    // Create the algorithm
    //
    ControllerModule* controllerMod = new ControllerModule(stream);
    Controller::Ref controller = controllerMod->getTask();
    controller->setTaskIndex(0);

    // Create one input and one output. Be sure that the output will properly connect to the Sink above.
    //
    assertEqual(0, stream->push(controllerMod));
    controller->addInputChannel(Channel("main", "Video"));
    sink->addInputChannel(Channel("input", "Video"));
    controller->addOutputChannel(Channel("output", "Video", sink));
    assertTrue(controller->openAndInit("Clamp"));

    Clamp* alg = dynamic_cast<Clamp*>(controller->getAlgorithm());
    assertTrue(alg);

    alg->setRange(kMin, kMax);
    assertTrue(controller->injectProcessingStateChange(ProcessingState::kRun));

    for (int messageCounter = 0; messageCounter < kMaxMessageCount; ++messageCounter) {
	VMEDataMessage vme;
	vme.header.azimuth = 0;
	vme.header.pri = messageCounter;
	Video::Ref main(Video::Make("test", vme, kMessageSize));
	for (int index = 0; index < kMessageSize; ++index)
	    main->push_back(index - kMessageSize + messageCounter);
	assertTrue(controller->putInChannel(main, 0));
    }

    // Run the ACE event loop until stopped by the ShutdownMonitor
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    // Uncomment the following to always fail the test and see the log results.
    // assertTrue(false);
}

bool
Test::testOutput(const Video::Ref& msg)
{
    assertEqual(size_t(kMessageSize), msg->size());
    for (int index = 0; index < msg->size(); ++index) {

	// Calculate what the clamped values should be and compare.
	//
	int v = index - kMessageSize + messageCounter_;
	if (v < kMin) v = kMin;
	else if (v > kMax) v = kMax;
	assertEqual(v, msg[index]);
    }

    return ++messageCounter_ == kMaxMessageCount;
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
