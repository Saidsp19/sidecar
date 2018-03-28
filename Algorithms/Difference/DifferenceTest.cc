#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "IO/MessageManager.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/Stream.h"

#include "Logger/Log.h"
#include "Messages/Video.h"
#include "UnitTest/UnitTest.h"

#include "Difference.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("Difference") {}
    void test();
    bool testOutput(const Video::Ref& output);
};

struct Sink : public Task {
    using Ref = boost::shared_ptr<Sink>;

    static Logger::Log& Log()
    {
        static Logger::Log& log_ = Logger::Log::Find("DifferenceTest.Sink");
        return log_;
    }

    static auto Make()
    {
        Ref ref(new Sink);
        return ref;
    }

    Sink() : Task(true), test_(0) {}

    void setTest(Test* test) { test_ = test; }

    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
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

    Test* test_;
};

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

    // Create two inputs and one output. Be sure that the output will properly connect to the Sink above.
    //
    assertEqual(0, stream->push(controllerMod));
    controller->addInputChannel(Channel("one", "Video"));
    controller->addInputChannel(Channel("two", "Video"));

    sink->addInputChannel(Channel("input", "Video"));
    controller->addOutputChannel(Channel("output", "Video", sink));
    assertTrue(controller->openAndInit("Difference"));

    Difference* alg = dynamic_cast<Difference*>(controller->getAlgorithm());
    assertTrue(alg);
    alg->setBufferSize(1);

    assertTrue(controller->injectProcessingStateChange(ProcessingState::kRun));

    int16_t init[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (int index = 0; index < 2; ++index) {
        VMEDataMessage vme;
        vme.header.azimuth = index;
        Video::Ref msg(Video::Make("test", vme, init, init + 10));
        assertTrue(controller->putInChannel(msg, index));
    }

    // Run the ACE event loop until stopped by the Sink.
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    // Uncomment the following to fail the test and see the log results.
    // assertTrue(false);
}

bool
Test::testOutput(const Video::Ref& msg)
{
    assertEqual(size_t(10), msg->size());
    Video::const_iterator pos = msg->begin();
    assertEqual(0, *pos++);
    assertEqual(0, *pos++);
    assertEqual(0, *pos++);
    assertEqual(0, *pos++);
    assertEqual(0, *pos++);
    assertEqual(0, *pos++);
    assertEqual(0, *pos++);
    assertEqual(0, *pos++);
    assertEqual(0, *pos++);
    assertEqual(0, *pos++);
    assertTrue(pos == msg->end());

    return true;
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
