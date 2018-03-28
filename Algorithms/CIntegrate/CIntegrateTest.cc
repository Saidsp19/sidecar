#include "ace/Reactor.h"

#include "Algorithms/Controller.h"
#include "IO/Decoder.h"
#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "IO/Task.h"
#include "Logger/Log.h"
#include "Messages/Video.h"
#include "UnitTest/UnitTest.h"

#include "CIntegrate.h"

using namespace SideCar::Algorithms;
using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() : UnitTest::TestObj("CIntegrate"), messageCounter_(4, 0), totalCount_(0) {}

    void test();

    bool testOutput(const Video::Ref& output, int channel);

    std::vector<int> messageCounter_;
    int totalCount_;
    static const int kMaxMessageCount;
    static const int kMessageSize;
};

const int Test::kMaxMessageCount = 1;
const int Test::kMessageSize = 20;

struct Sink : public Task {
    using Ref = boost::shared_ptr<Sink>;

    static Logger::Log& Log();

    static Ref Make()
    {
        Ref ref(new Sink());
        return ref;
    }

    Sink() : Task(), test_(0) {}

    void setTest(Test* test)
    {
        test_ = test;
        setUsingData(true);
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

    int channel = data->msg_priority();
    LOGERROR << getTaskName() << " channel: " << channel << std::endl;

    MessageManager mgr(data);
    if (mgr.hasNative()) {
        LOGERROR << "native type: " << mgr.getNativeMessageType() << std::endl;
        if (mgr.getNativeMessageType() == MetaTypeInfo::Value::kVideo) {
            Video::Ref msg(mgr.getNative<Video>());
            LOGERROR << msg->dataPrinter() << std::endl;
            if (test_->testOutput(msg, channel)) {
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
    Logger::ProcLog log("test", Logger::Log::Root());

    Stream::Ref stream(Stream::Make("test"));

    // Create tasks that will look at the output of the algorithm
    //
    TModule<Sink>* ctm = new TModule<Sink>(stream);
    assertEqual(0, stream->push(ctm));
    Sink::Ref sink = ctm->getTask();
    sink->setTaskIndex(1);
    sink->setTaskName("Sink");
    sink->setTest(this);

    // Add four separate input channels.
    //
    sink->addInputChannel(Channel("results", "Video"));
    sink->addInputChannel(Channel("maxAmp", "Video"));
    sink->addInputChannel(Channel("dopplerBinOutput", "Video"));
    sink->addInputChannel(Channel("rgb", "Video"));

    // Create the algorithm
    //
    ControllerModule* controllerMod = new ControllerModule(stream);
    Controller::Ref controller = controllerMod->getTask();
    controller->setTaskIndex(0);

    assertEqual(0, stream->push(controllerMod));

    controller->addInputChannel(Channel("main", "Video"));

    // Create output channels that are properly linked to the input channels of the Sink device above.
    //
    controller->addOutputChannel(Channel("results", "Video", sink, 0));
    controller->addOutputChannel(Channel("maxAmp", "Video", sink, 1));
    controller->addOutputChannel(Channel("dopplerBinOutput", "Video", sink, 2));
    controller->addOutputChannel(Channel("rgb", "Video", sink, 3));

    assertTrue(controller->openAndInit("CIntegrate"));

    CIntegrate* alg = dynamic_cast<CIntegrate*>(controller->getAlgorithm());
    assertTrue(alg);
    alg->setNumPRIs(4);
    alg->setSlidingWindowShift(1);
    alg->setEncoding(CIntegrate::kRGB);
    assertTrue(controller->injectProcessingStateChange(ProcessingState::kRun));

    // We need 64 messages before we get anything out of the stock CIntegrate algorithm.
    //
    int16_t mainData[5][20] = {
        {0, 0, 5, 0, 9, 0, 6, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 5, 0, 9, 0, 6, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 5, 0, 9, 0, 6, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 5, 0, 9, 0, 6, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 5, 0, 9, 0, 6, 0, 2, 0, 1, 0, 0, 0, 0, 0},
    };

    for (int index = 0; index < 5; ++index) {
        VMEDataMessage vme;
        vme.header.azimuth = 0;
        vme.header.pri = index;
        Video::Ref main(Video::Make("test", vme, mainData[index], mainData[index] + 20));
        assertTrue(controller->putInChannel(main, 0));
    }

    // Run the ACE event loop until stopped by the ShutdownMonitor
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    stream->close();

    // Uncomment the following to always fail the test and see the log results. assertTrue(false);
}

int16_t fftData[8][10] = {
    {
        0,
        2,
        7,
        8,
        4,
        2,
        0,
        0,
        0,
        0,
    },
    {
        0,
        2,
        1,
        1,
        1,
        0,
        0,
        0,
        0,
        0,
    },
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    },
    {
        0,
        2,
        1,
        1,
        1,
        0,
        0,
        0,
        0,
        0,
    },
    {
        0,
        1,
        5,
        7,
        6,
        3,
        1,
        0,
        0,
        0,
    },
    {
        0,
        1,
        2,
        1,
        1,
        1,
        0,
        0,
        0,
        0,
    },
    {
        0,
        1,
        2,
        0,
        2,
        1,
        0,
        0,
        0,
        0,
    },
    {
        0,
        1,
        2,
        1,
        1,
        1,
        0,
        0,
        0,
        0,
    },
};

int16_t maxAmpData[2][20] = {
    {
        0, 0, 2, 0, 7, 0, 7, 0, 4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        0, 0, 1, 0, 4, 0, 7, 0, 5, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    },
};

int16_t oneDopData[2][20] = {
    {
        0, 0, 2, 0, 7, 0, 7, 0, 4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        0, 0, 1, 0, 4, 0, 7, 0, 5, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    },
};

int16_t rgbData[2][10] = {
    {
        0,
        0,
        0,
        32,
        0,
        0,
        0,
        0,
        0,
        0,
    },
    {
        0,
        0,
        0,
        32,
        0,
        0,
        0,
        0,
        0,
        0,
    },
};

bool
Test::testOutput(const Video::Ref& msg, int channel)
{
    int index = messageCounter_[channel]++;

    int16_t* p;
    switch (channel) {
    case 0:
        p = fftData[index];
        assertEqual(10, msg->size());
        break;
    case 1:
        p = maxAmpData[index];
        assertEqual(20, msg->size());
        break;
    case 2:
        p = oneDopData[index];
        assertEqual(20, msg->size());
        break;
    case 3:
        p = rgbData[index];
        assertEqual(10, msg->size());
        break;
    }

    for (size_t index = 0; index < msg->size(); ++index) { assertEqual(p[index], msg[index]); }

    return ++totalCount_ == 4 + 4 + 2 + 2 + 2;
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
