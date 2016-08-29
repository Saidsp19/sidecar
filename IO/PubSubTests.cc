#include "ace/Reactor.h"

#include "IO/Module.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/Stream.h"
#include "Logger/Log.h"
#include "Messages/Header.h"
#include "Time/TimeStamp.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "MulticastDataPublisher.h"
#include "MulticastDataSubscriber.h"
#include "TCPDataPublisher.h"
#include "TCPDataSubscriber.h"

#include "MessageManager.h"

using namespace SideCar;
using namespace SideCar::IO;
using namespace SideCar::Messages;

class Message : public Header
{
public:
    using Ref = boost::shared_ptr<Message>;

    static const MetaTypeInfo& GetMetaTypeInfo();

    static Header::Ref Loader(ACE_InputCDR& cdr)
	{
	    return Make(cdr);
	}

    static Ref Make(const std::string& value)
	{
	    Ref ref(new Message(value));
	    return ref;
	}

    static Ref Make(ACE_InputCDR& cdr)
	{
	    Ref ref(new Message(cdr));
	    return ref;
	}

    const std::string& getValue() const { return value_; }

    ACE_InputCDR& load(ACE_InputCDR& cdr)
	{
	    Header::load(cdr);
	    cdr >> value_;
	    return cdr;
	}

    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const
	{
	    Header::write(cdr);
	    cdr << value_;
	    return cdr;
	}

private:
    Message(ACE_InputCDR& cdr) : Header(GetMetaTypeInfo()), value_("")
	{ load(cdr); }

    Message(const std::string& value)
	: Header("test", GetMetaTypeInfo()), value_(value) {}

    std::string value_;

    static MetaTypeInfo metaTypeInfo_;
};

MetaTypeInfo Message::metaTypeInfo_(MetaTypeInfo::Value::kUnassigned, "Message", &Message::Loader, 0);

const MetaTypeInfo&
Message::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

struct EmitterTask : public Task
{
    enum {
	kNumMessages = 20,
	kTimeout = 15,
    };

    using Ref = boost::shared_ptr<EmitterTask>;

    static Logger::Log& Log()
	{
	    static Logger::Log& log_ = Logger::Log::Find("EmitterTask");
	    return log_;
	}

    static Ref Make() { Ref ref(new EmitterTask); return ref; }

    EmitterTask() : Task(), timerId_(-1), count_(0) {}

    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
	{ return put_next(data, timeout) != -1; }

    void open()
	{
	    if (! reactor()) reactor(ACE_Reactor::instance());
	    setUsingData(false);
	    ACE_Time_Value delay(0,  0);
	    ACE_Time_Value repeat(0, 10);
	    timerId_ = reactor()->schedule_timer(this, 0, delay, repeat);
	    startTime_ = Time::TimeStamp::Now();
	}

    int close(u_long flags = 0)
	{
	    if (timerId_ != -1)
		reactor()->cancel_timer(timerId_);
	    return Task::close(flags);
	}

    int handle_timeout(const ACE_Time_Value& timeout, const void* arg);

    void setUsingData(bool isOpen)
	{
	    Logger::ProcLog log("setUsingData", Log());
	    LOGERROR << "setUsingData: " << isOpen << std::endl;
	    Task::setUsingData(isOpen);
	}

    long timerId_;
    int count_;
    Time::TimeStamp startTime_;
};

int
EmitterTask::handle_timeout(const ACE_Time_Value& duration, const void* arg)
{
    static Logger::ProcLog log("handle_timeout", Log());

    // If we are accepting data, then the CounterTask is up and running, so begin sending messages to it. Once
    // the CounterTask has received kNumMessages, it will gracefull shut things down.
    //
    if (isUsingData()) {
	if (count_ < kNumMessages) {
	    ++count_;
	    LOGINFO << "message " << count_ << std::endl;
	    std::string big(4096, '!');
	    Message::Ref ref(Message::Make(big));
	    MessageManager mgr(ref);
	    if (put_next(mgr.getMessage(), 0) == -1) {
		LOGERROR << "failed to put message" << std::endl;
	    }
	    else {
		return 0;
	    }
	}
    }

    // If we got here, then either the CounterTask is not up and running (yet) or we failed the put_next()
    // method above. Check to see if we've run out of time for the test, and if so, gracefully shut things down.
    //
    Time::TimeStamp delta(Time::TimeStamp::Now());
    delta -= startTime_;
    if (delta.asDouble() > kTimeout) {
	LOGERROR << "*** timed-out" << std::endl;
	LOGINFO << "*** shutting down server" << std::endl;
	ACE_Reactor::instance()->end_reactor_event_loop();
    }

    return 0;
}

struct CounterTask : public Task
{
    using Ref = boost::shared_ptr<CounterTask>;

    static Logger::Log& Log()
	{
	    static Logger::Log& log_ = Logger::Log::Find("EmitterTask");
	    return log_;
	}

    static Ref Make() { Ref ref(new CounterTask); return ref; }

    CounterTask() : Task(), count_(0) {}

    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    void setUsingData(bool isOpen)
	{ Task::setUsingData(true); }

    int count_;
};

bool
CounterTask::deliverDataMessage(ACE_Message_Block* data,
                                ACE_Time_Value* timeout)
{
    Logger::ProcLog log("deliverDataMessage", Log());

    MessageManager manager(data);
    LOGDEBUG << "*** " << count_ << " " << manager.getMessageType()
	     << std::endl;

    if (! manager.hasNativeMessageType(MetaTypeInfo::Value::kUnassigned)) {
	LOGERROR << "message does not have expected channel ID" << std::endl;
	ACE_Reactor::instance()->end_reactor_event_loop();
	return false;
    }

    Message::Ref msg = manager.getNative<Message>();
    LOGDEBUG << "msg.header: " << msg->getGloballyUniqueID() << std::endl;

    ++count_;
    if (count_ == EmitterTask::kNumMessages) {
	LOGINFO << "*** shutting down server" << std::endl;
	ACE_Reactor::instance()->end_reactor_event_loop();
    }

    return true;
}

struct Test : public UnitTest::ProcSuite<Test>
{
    Test() : UnitTest::ProcSuite<Test>(this, "PubSub")
	{
	    add("UDP", &Test::testUDP);
	    add("TCP", &Test::testTCP);
	}

    void testTCP();
    void testUDP();
};

void
Test::testTCP()
{
    ACE_Reactor::instance()->reset_reactor_event_loop();

    // Create a stream with the following components: [TCPDataSubscriber, Counter, Emitter, TCPDataPublisher]
    // The publisher will send data from the Emitter, and the Counter will count the number of messages received
    // by the subscriber.
    //
    Stream::Ref stream(Stream::Make("TCP"));
    TCPDataPublisherModule* pm = new TCPDataPublisherModule(stream);
    stream->push(pm);
    TCPDataPublisher::Ref publisher(pm->getTask());
    assertEqual(true, publisher->openAndInit("Message", "TestPubTCP"));

    TModule<EmitterTask>* etm = new TModule<EmitterTask>(stream);
    stream->push(etm);
    etm->getTask()->open();

    Channel ch1("output", "Message", etm->getTask());
    ch1.setSender(etm->getTask());
    etm->getTask()->addOutputChannel(ch1);
    pm->getTask()->addInputChannel(ch1);

    TModule<CounterTask>* ctm = new TModule<CounterTask>(stream);
    stream->push(ctm);

    TCPDataSubscriberModule* sm = new TCPDataSubscriberModule(stream);
    stream->push(sm);

    TCPDataSubscriber::Ref subscriber(sm->getTask());
    assertEqual(true, subscriber->openAndInit("Message", "TestPubTCP"));

    Channel ch2("output", "Message", ctm->getTask());
    ch2.setSender(sm->getTask());
    sm->getTask()->addOutputChannel(ch2);
    ctm->getTask()->addInputChannel(ch2);

    ctm->getTask()->setUsingData(true);

    // Let 'er rip!
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    assertEqual(int(EmitterTask::kNumMessages), etm->getTask()->count_);
    assertEqual(int(EmitterTask::kNumMessages), ctm->getTask()->count_);

    stream->close();

    // Uncomment the following to see the log messages when there are no failures assertFalse(true);
}

void
Test::testUDP()
{
    ACE_Reactor::instance()->reset_reactor_event_loop();

    // Create a stream with the following components: [MulticastDataSubscriber, Counter, Emitter,
    //  MulticastDataPublisher] The publisher will send data from the Emitter, and the Counter will count the
    //  number of messages received by the subscriber.
    //
    Stream::Ref stream(Stream::Make("UDP"));
    MulticastDataPublisherModule* pm = new MulticastDataPublisherModule(stream);
    stream->push(pm);
    MulticastDataPublisher::Ref publisher(pm->getTask());
    assertEqual(true, publisher->openAndInit("Message", "TestPubUDP", "239.255.0.1"));

    TModule<EmitterTask>* etm = new TModule<EmitterTask>(stream);
    stream->push(etm);
    etm->getTask()->open();

    Channel ch1("output", "Message", etm->getTask());
    ch1.setSender(etm->getTask());
    etm->getTask()->addOutputChannel(ch1);
    pm->getTask()->addInputChannel(ch1);

    TModule<CounterTask>* ctm = new TModule<CounterTask>(stream);
    stream->push(ctm);

    MulticastDataSubscriberModule* sm = new MulticastDataSubscriberModule(stream);
    stream->push(sm);
    MulticastDataSubscriber::Ref subscriber(sm->getTask());
    assertEqual(true, subscriber->openAndInit("Message", "TestPubUDP"));

    Channel ch2("output", "Message", ctm->getTask());
    ch2.setSender(sm->getTask());
    sm->getTask()->addOutputChannel(ch2);
    ctm->getTask()->addInputChannel(ch2);

    ctm->getTask()->setUsingData(true);

    // Let 'er rip!
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    assertEqual(int(EmitterTask::kNumMessages), etm->getTask()->count_);
    assertEqual(int(EmitterTask::kNumMessages), ctm->getTask()->count_);

    stream->close();

    // Uncomment the following to see the log messages when there are no failures assertFalse(true);
}

int
main(int argc, const char* argv[])
{
    Logger::Log& log = Logger::Log::Find("root");
    log.setPriorityLimit(Logger::Priority::kDebug);

    for (int repetition = 0; repetition < 4; ++repetition) {
	LOGINFO << "repetition: " << repetition << std::endl;
	int rc = Test().mainRun();
	if (rc) {
	    LOGERROR << "failed in repetition " << (repetition + 1)
		     << std::endl;
	    return rc;
	}
    }

    return 0;
}
