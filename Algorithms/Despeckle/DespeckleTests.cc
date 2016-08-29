#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/Stream.h"
#include "IO/Task.h"

#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"

#include "Despeckle.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

constexpr int kNumRows = 5;
constexpr int kNumCols = 6;
constexpr int16_t kTestData[kNumRows][kNumCols] = {
    { 5, 5, 5, 5, 5, 5 },
    { 4, 9, 4, 4, 0, 4 },
    { 3, 3, 3, 0, 3, 0 },
    { 2, 9, 2, 2, 9, 0 },
    { 1, 1, 1, 1, 1, 1 }
};

struct Test : public UnitTest::TestObj
{
    Test() : UnitTest::TestObj("Despeckle"), messageCounter_(0) {}
    void test();
    bool testOutput(const Video::Ref& msg);
    int messageCounter_;
};

struct Sink : public Task
{
    using Ref = boost::shared_ptr<Sink>;

    static Logger::Log& Log();

    static Ref Make() { Ref ref(new Sink); return ref; }

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
    MessageManager mgr(data);

    LOGTIN << mgr.hasNative() << std::endl;

    if (mgr.hasNative()) {
	LOGERROR << "metaType: " << mgr.getNativeMessageType() << std::endl;
	if (mgr.getNativeMessageType() == MetaTypeInfo::Value::kVideo) {
	    Video::Ref msg(mgr.getNative<Video>());
	    LOGINFO << msg->dataPrinter() << std::endl;
	    if (test_->testOutput(msg)) {
		LOGINFO << "shutting down server" << std::endl;
		ACE_Reactor::instance()->end_reactor_event_loop();
	    }
	}
    }

    LOGTOUT << true << std::endl;
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
    assertTrue(controller->openAndInit("Despeckle"));

    Despeckle* alg = dynamic_cast<Despeckle*>(controller->getAlgorithm());
    assertTrue(alg);
    alg->setVarianceMultiplier(1.0);
    assertTrue(controller->injectProcessingStateChange(ProcessingState::kRun));

//    stream.put(ProcessingStateChangeRequest(ProcessingState::kRun).getWrapped());

    for (int messageCounter = 0; messageCounter < kNumRows; ++messageCounter) {
	VMEDataMessage vme;
	vme.header.azimuth = 0;
        Video::Ref msg(Video::Make("test", vme, kTestData[messageCounter], kTestData[messageCounter] + kNumCols));
        assertTrue(controller->putInChannel(msg, 0));
    }

    // Run the ACE event loop until stopped by the ShutdownMonitor
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    assertEqual(3, messageCounter_);

    // Uncomment the following to always fail the test and see the log results.
    // assertTrue(false);
}

bool
Test::testOutput(const Video::Ref& msg)
{
    Logger::Log& log = Logger::Log::Find("Test.testOutput");
    LOGTIN << std::endl;
    
    Video::const_iterator pos = msg->begin();
    assertEqual(size_t(kNumCols), msg->size());
    switch (messageCounter_) {
    case 0:
        assertEqual(4, *pos++);
        assertEqual(4, *pos++);
        assertEqual(4, *pos++);
        assertEqual(4, *pos++);
        assertEqual(0, *pos++);
        assertEqual(4, *pos++);
	assertTrue(pos == msg->end());
        break;
        
    case 1:
        assertEqual(3, *pos++);
        assertEqual(3, *pos++);
        assertEqual(3, *pos++);
        assertEqual(0, *pos++);
        assertEqual(3, *pos++);
        assertEqual(0, *pos++);
	assertTrue(pos == msg->end());
        break;
        
    case 2:
        assertEqual(2, *pos++);
        assertEqual(2, *pos++);
        assertEqual(2, *pos++);
        assertEqual(2, *pos++);
        assertEqual(1, *pos++);
        assertEqual(0, *pos++);
	assertTrue(pos == msg->end());
        break;
    }

    return ++messageCounter_ == 3;
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
