#include "ace/Reactor.h"

#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "IO/Task.h"

#include "Algorithms/Controller.h"
#include "Logger/Log.h"
#include "Messages/Video.h"
#include "UnitTest/UnitTest.h"

#include "SimpleOp.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    static Logger::Log& Log();

    Test() : UnitTest::TestObj("SimpleOp") {}
    void test();
    void testOutput(size_t counter, const Video::Ref& output);
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

    Sink() : Task(true), count_(0), test_(0) {}

    void setTest(Test* test) { test_ = test; }
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    int count_;
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
    LOGERROR << "count: " << count_ << " message type: "
	     << mgr.getMessageType() << std::endl;

    if (mgr.hasNative()) {
	LOGERROR << "metaType: " << mgr.getNativeMessageType() << std::endl;
	if (mgr.getNativeMessageType() == MetaTypeInfo::Value::kVideo) {
	    Video::Ref msg(mgr.getNative<Video>());
	    LOGERROR << msg->dataPrinter() << std::endl;
	    test_->testOutput(count_++, msg);
	    if (count_ == 1) {
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

    // Create the algorithm
    //
    ControllerModule* controllerMod = new ControllerModule(stream);
    assertEqual(0, stream->push(controllerMod));
    Controller::Ref controller = controllerMod->getTask();
    controller->setTaskIndex(0);

    sink->addInputChannel(Channel("input", "Video"));

    // Create three inputs and one output. Be sure that the output will properly connect to the Sink above.
    //
    controller->addInputChannel(Channel("one", "Video"));
    controller->addInputChannel(Channel("two", "Video"));
    controller->addInputChannel(Channel("three", "Video"));
    controller->addOutputChannel(Channel("output", "Video", sink));

    assertTrue(controller->openAndInit("SimpleOp"));
    assertTrue(controller->injectProcessingStateChange(ProcessingState::kRun));
    SimpleOp* alg = dynamic_cast<SimpleOp*>(controller->getAlgorithm());
    alg->setOperation(SimpleOp::kProdOp);

    // Send data to the algorithm
    //
    VMEDataMessage vme;
    vme.header.azimuth = 0;
    {
	short init[] = { 1, 2, 3, 4, 5 };
	Video::Ref msg(Video::Make("test", vme, init, init + 5));
	assertTrue(controller->putInChannel(msg, 0));
    }
    {
	short init[] = { 6, 7, 8, 9, 10 };
	Video::Ref msg(Video::Make("test", vme, init, init + 5));
	assertTrue(controller->putInChannel(msg, 1));
    }
    {
	short init[] = { 8, 7, 6, 5, 4, 3, 2, 1 };
	Video::Ref msg(Video::Make("test", vme, init, init + 8));
	assertTrue(controller->putInChannel(msg, 2));
    }

    // Now run.
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    // Uncomment the following to fail the test and see the log results. assertTrue(false);
}

void
Test::testOutput(size_t count, const Video::Ref& msg)
{
    assertEqual(size_t(5), msg->size());
    assertEqual(48, msg[0]);
    assertEqual(98, msg[1]);
    assertEqual(144, msg[2]);
    assertEqual(180, msg[3]);
    assertEqual(200, msg[4]);
}

int
main(int argc, char** argv)
{
    return Test().mainRun();
}
