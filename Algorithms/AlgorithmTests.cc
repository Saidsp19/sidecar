#include "ace/Event_Handler.h"
#include "ace/Reactor.h"

#include "IO/MessageManager.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/Stream.h"
#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "UnitTest/UnitTest.h"
#include "Utils/VsipVector.h"
#include "Utils/Utils.h"

#include "Algorithm.h"
#include "Controller.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;
using namespace SideCar::Parameter;

struct AlgorithmTest : public UnitTest::TestObj {
    AlgorithmTest() : TestObj("Algorithm") {}

    void test();
};

// Dummy algorithm we will use for testing purposes. Does nothing but emits messages it receives
//
class Blah : public Algorithm, ACE_Event_Handler {
public:

    static Logger::Log& Log()
        {
            static Logger::Log& log_ = Logger::Log::Find("Blah");
            return log_;
        }

    Blah(Controller& controller, Logger::Log& log) :
        Algorithm(controller, log),
        foo_(IntValue::Make("foo", "foo test", 1)),
        bar_(DoubleValue::Make("bar", "bar test", 1.0))
        {}

    bool startup()
        {
            Logger::ProcLog log("startup", Log());
            LOGINFO << "STARTUP" << std::endl;
            registerProcessor<Blah, Video>("input", &Blah::process);
            registerParameter(foo_);
            registerParameter(bar_);
            return Algorithm::startup();
        }

    bool reset()
        {
            Logger::ProcLog log("reset", Log());
            LOGINFO << "RESET" << std::endl;
            return Algorithm::reset();
        }
    
    bool process(const Video::Ref& msg)
        {
            Logger::ProcLog log("process", Log());
            
            // Obtain a new message to use for our output values.
            //
            Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
            out->resize(msg->size());

            // Use VSIPL to do the conversion for us. First, we need to create VSIPL vectors that use our message data.
            //
            VsipVector<Messages::Video> vMsg(*msg);
            VsipVector<Messages::Video> vOut(*out);

            vMsg.admit(true);  // Tell VSIPL to use data from msg
            vOut.admit(false); // Tell VSIPL to ignore data from out

            // Perform the scaling.
            //
            vOut.v = bar_->getValue() * vMsg.v;

            vMsg.release(false); // Don't flush data from VSIPL to msg
            vOut.release(true);  // Do flush data from VSIPL to out

            // Send out on the default output channel, and return the result to the controller.
            //
            bool rc = send(out);
            LOGDEBUG << "rc: " << rc << std::endl;
            return rc;
        }

    bool stop()
        {
            Logger::ProcLog log("stop", Log());
            LOGINFO << "STOP" << std::endl;
            return Algorithm::stop();
        }

    IntValue::Ref foo_;
    DoubleValue::Ref bar_;
};

struct Sink : public IO::Task {
    using Ref = boost::shared_ptr<Sink>;

    static Logger::Log& Log()
        {
            static Logger::Log& log_ = Logger::Log::Find("Sink");
            return log_;
        }

    static Ref Make()
        {
            Ref ref(new Sink);
            return ref;
        }

    Sink() : Task(true), msgs_(), expected_(10), watchdogTimer_(-1)
        {
            startTimer();
        }

    void startTimer()
        {
            Logger::ProcLog log("startTimer", Log());
            LOGINFO << "START_TIMER" << std::endl;
            stopTimer();
            const ACE_Time_Value delay(10);
            watchdogTimer_ = ACE_Reactor::instance()->schedule_timer(this, 0, delay);
        }

    void stopTimer()
        {
            Logger::ProcLog log("stopTimer", Log());
            LOGINFO << "STOP_TIMER - " << watchdogTimer_ << std::endl;
            if (watchdogTimer_ != -1) {
                ACE_Reactor::instance()->cancel_timer(watchdogTimer_);
                watchdogTimer_ = -1;
            }
        }

    int handle_timeout(const ACE_Time_Value&, const void*)
        {
            Logger::ProcLog log("handle_timeout", Log());
            LOGINFO << "HANDLE_TIMEOUT" << std::endl;
            ACE_Reactor::instance()->end_reactor_event_loop();
            watchdogTimer_ = -1;
            return 0;
        }

    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
        {
            Logger::ProcLog log("deliverDataMessage", Log());
            IO::MessageManager mgr(data);
            if (mgr.hasNative()) {
                LOGINFO << "metaType: " << mgr.getNativeMessageType() << std::endl;
                if (mgr.getNativeMessageType() == MetaTypeInfo::Value::kVideo) {
                    msgs_.push_back(mgr.getNative<Video>());
                    if (--expected_ == 0) {
                        stopTimer();
                        ACE_Reactor::instance()->end_reactor_event_loop();
                    }
                    else {
                        startTimer();
                    }
                }
            }

            return true;
        }

    std::vector<Video::Ref> msgs_;
    long watchdogTimer_;
    int expected_;
};

void
AlgorithmTest::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kInfo);

    // Create a processing stream and add processing elements
    //
    IO::Stream::Ref stream(IO::Stream::Make("AlgorithmTest"));
    auto ctm = new IO::TModule<Sink>(stream);
    assertEqual(0, stream->push(ctm));
    Sink::Ref sink = ctm->getTask();
    sink->setTaskName("Sink");
    sink->setTaskIndex(1);
    
    // Create an algorithm Controller and our mock Algorithm and add to the stream
    //
    ControllerModule* module = new ControllerModule(stream);
    assertEqual(0, stream->push(module));
    Controller::Ref controller = module->getTask();
    controller->setTaskIndex(0);

    sink->addInputChannel(IO::Channel("input", "Video"));

    controller->addInputChannel(IO::Channel("input", "Video"));
    controller->addOutputChannel(IO::Channel("output", "Video", sink));

    Blah* blah = new Blah(*controller, Logger::Log::Root());
    assertTrue(controller->openAndInit("blah", "", blah));
    blah->bar_->setValue(0.5);
    assertTrue(controller->injectProcessingStateChange(IO::ProcessingState::kAutoDiagnostic));

    VMEDataMessage vme;
    std::vector<Video::Ref> inputs;
    for (int count = 0; count < 10; ++count) {
        Video::Ref msg(Video::Make("hi", vme, 3));
        msg->push_back(100);
        msg->push_back(200);
        msg->push_back(300);
        inputs.push_back(msg);
        assertTrue(controller->putInChannel(msg, 0));
    }

    ACE_Reactor::instance()->run_reactor_event_loop();

    assertTrue(sink->msgs_.size() == 10);

    // Validate that the output from the algorithm is 0.5 of the input
    //
    for (size_t index = 0; index < sink->msgs_.size(); ++index) {
        auto input = inputs[index];
        auto output = sink->msgs_[index];
        assertEqual(input[0] * blah->bar_->getValue(), output[0]);
        assertEqual(input[1] * blah->bar_->getValue(), output[1]);
        assertEqual(input[2] * blah->bar_->getValue(), output[2]);
    }
}

int
main(int argc, char** argv)
{
    return (new AlgorithmTest)->mainRun();
}
