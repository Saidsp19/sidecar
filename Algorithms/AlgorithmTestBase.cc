// -*- C++ -*-

#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "IO/Task.h"
#include "Logger/Log.h"
#include "Messages/Video.h"
#include "UnitTest/UnitTest.h"

using namespace SideCar::Algorithms;

TestBase::TestBase(const std::string& algo, const std::string& inputType, const std::string& outputType,
                   int messageLimit) :
    algo_(algo),
    inputType_(inputType), outputType_(outputType), messageCounter_(0), messageLimit_(messageLimit), alwaysFail_(false),
    log_(Logger::Log::Find(algo + "Test"))
{
}

bool
Sink::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    Logger::ProcLog log("Sink.deliverDataMessage", test_->log_);
    IO::MessageManager mgr(data);
    LOGTIN << mgr.hasNative() << std::endl;
    if (mgr.hasNative()) {
        LOGINFO << "metaType: " << mgr.getNativeMessageType() << std::endl;
        test_->validateOutput(mgr);
        if (++messageCounter_ == test_->messageLimit_) {
            LOGINFO << "shutting down server" << std::endl;
            ACE_Reactor::instance()->end_reactor_event_loop();
        }
    }

    return true;
}

void
TestBase::test()
{
    // Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Stream stream(algo_, 0);

    // Create a sink task that will see the output of the algorithm
    //
    TModule<Sink>* ctm = new TModule<Sink>(&stream);
    assertEqual(0, stream.push(ctm));

    Sink::Ref sink = ctm->getTask();
    sink->setTaskIndex(1);
    sink->setTest(this);

    // Create the algorithm
    //
    ControllerModule* controllerMod = new ControllerModule(&stream);
    Controller::Ref controller = controllerMod->getTask();
    controller->setTaskIndex(0);

    // Create one input and one output. Be sure that the output will properly connect to the Sink above.
    //
    assertEqual(0, stream.push(controllerMod));
    controller->addInputChannel(Channel(Task::Ref(), "main", inputType_));
    sink->addInputChannel(Channel(controller, "input", outputType_));
    controller->addOutputChannel(Channel(Task::Ref(), "output", outputType_, sink, 0));
    assertTrue(controller->openAndInit(algo_));

    // Configure algorithm if necessary
    //
    configureAlgorithm(controller);

    // Enter RUN state for proper processing flow
    //
    assertTrue(controller->injectProcessingStateChange(ProcessingState::kRun));

    // Submit test messages to algorithm
    //
    injectMessages(controller);

    // Run the ACE event loop -- *NOTE* this blocks until stopped by the Sink
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    if (alwaysFail_) assertTrue(false);
}

// Examples of methods

#if 0

void
TestBase::configureAlgorithm(const Controller::Ref& controller)
{
  Clamp* alg = dynamic_cast<Clamp*>(controller->getAlgorithm());
  assertTrue(alg);
  alg->setRange(kMin, kMax);
}

void
TestBase::injectMessages(const Controller::Ref& controller)
{
  for (int messageCounter = 0; messageCounter < kMaxMessageCount; ++messageCounter) {
    VMEDataMessage vme;
    vme.header.azimuth = 0;
    vme.header.pri = messageCounter;
    Video::Ref main(Video::Make("test", vme, kMessageSize));
    for (int index = 0; index < kMessageSize; ++index)
      main->push_back(index - kMessageSize + messageCounter);
    assertTrue(controller->putInChannel(main, 0));
  }


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

#endif
