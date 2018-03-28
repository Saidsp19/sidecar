#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "IO/Task.h"
#include "Logger/Log.h"
#include "Messages/Video.h"
#include "UnitTest/UnitTest.h"

#include "Decimator.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() :
        UnitTest::TestObj("Decimator"), alg_(0), controller_(), messageCounter_(0), kMaxMessageCount(2),
        kMessageSize(12)
    {
    }

    void test();
    bool testOutput(const Video::Ref& output);

    Decimator* alg_;
    Controller::Ref controller_;
    int messageCounter_;
    const int kMaxMessageCount;
    const int kMessageSize;
};

struct Sink : public Task {
    using Ref = boost::shared_ptr<Sink>;

    static Logger::Log& Log();

    static Ref Make()
    {
        Ref ref(new Sink);
        return ref;
    }

    Sink() : Task(true), test_(0)
    {
        Logger::ProcLog log("Sink", Log());
        LOGERROR << std::endl;
    }

    ~Sink()
    {
        Logger::ProcLog log("~Sink", Log());
        LOGERROR << std::endl;
    }

    void setTest(Test* test)
    {
        Logger::ProcLog log("setTest", Log());
        test_ = test;
        LOGINFO << "test: " << test_ << std::endl;
    }

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
            LOGERROR << test_ << ' ' << test_->alg_ << std::endl;
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
    controller_ = controllerMod->getTask();
    controller_->setTaskIndex(0);

    // Create one input and one output. Be sure that the output will properly connect to the Sink above.
    //
    assertEqual(0, stream->push(controllerMod));
    controller_->addInputChannel(Channel("main", "Video"));
    sink->addInputChannel(Channel("input", "Video"));
    controller_->addOutputChannel(Channel("output", "Video", sink));
    assertTrue(controller_->openAndInit("Decimator"));

    alg_ = dynamic_cast<Decimator*>(controller_->getAlgorithm());
    assertTrue(alg_);

    alg_->setDecimationFactor(3);
    alg_->setIsIQ(false);

    assertTrue(controller_->injectProcessingStateChange(ProcessingState::kRun));

    VMEDataMessage vme;
    vme.header.azimuth = 0;
    vme.header.pri = 0;
    int16_t init[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    Video::Ref msg(Video::Make("test", vme, init, init + 12));
    assertTrue(controller_->putInChannel(msg, 0));

    // Run the ACE event loop until stopped by the ShutdownMonitor
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    // Uncomment the following to always fail the test and see the log results. assertTrue(false);
}

bool
Test::testOutput(const Video::Ref& msg)
{
    if (messageCounter_ == 0) {
        // Validate message content for decimated non-IQ message
        //
        assertEqual(size_t(4), msg->size());
        Video::const_iterator pos = msg->begin();
        assertEqual(1, *pos++);
        assertEqual(4, *pos++);
        assertEqual(7, *pos++);
        assertEqual(10, *pos++);
        assertTrue(pos == msg->end());

        // Now set the algorithm to decimate IQ data
        //
        alg_->setDecimationFactor(4);
        alg_->setIsIQ(true);

        // Submit the same message
        //
        VMEDataMessage vme;
        vme.header.azimuth = 0;
        vme.header.pri = 0;
        int16_t init[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
        Video::Ref msg(Video::Make("test", vme, init, init + 12));
        assertTrue(controller_->putInChannel(msg, 0));
    } else {
        // Validate message content for decimated IQ message
        //
        assertEqual(size_t(4), msg->size());
        Video::const_iterator pos = msg->begin();
        assertEqual(1, *pos++);
        assertEqual(2, *pos++);
        assertEqual(9, *pos++);
        assertEqual(10, *pos++);
        assertTrue(pos == msg->end());
    }

    return ++messageCounter_ == kMaxMessageCount;
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
