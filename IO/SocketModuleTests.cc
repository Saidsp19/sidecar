#include "ace/Reactor.h"

#include "IO/Module.h"
#include "IO/Stream.h"
#include "Logger/Log.h"
#include "Messages/Header.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "ClientSocketReaderTask.h"
#include "ClientSocketWriterTask.h"
#include "MessageManager.h"
#include "ServerSocketReaderTask.h"
#include "ServerSocketWriterTask.h"

using namespace SideCar;
using namespace SideCar::IO;
using namespace SideCar::Messages;

class Message : public Header {
public:
    using Ref = boost::shared_ptr<Message>;

    static const MetaTypeInfo& GetMetaTypeInfo();

    static Header::Ref Loader(ACE_InputCDR& cdr) { return Make(cdr); }

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
    Message(ACE_InputCDR& cdr) : Header(GetMetaTypeInfo()), value_("") { load(cdr); }

    Message(const std::string& value) : Header("test", GetMetaTypeInfo()), value_(value) {}

    std::string value_;

    static MetaTypeInfo metaTypeInfo_;
};

MetaTypeInfo Message::metaTypeInfo_(MetaTypeInfo::kUnassigned, "Message", &Message::Loader);

const MetaTypeInfo&
Message::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

struct TestTask : public Task {
    using Ref = boost::shared_ptr<TestTask>;

    enum Fluffy { kNumMessages = 9 };

    static Logger::Log& Log()
    {
        static Logger::Log& log = Logger::Log::Find("TestTask");
        return log;
    }

    static Ref Make()
    {
        Ref ref(new TestTask);
        return ref;
    }

    TestTask() : Task(), timerId_(-1), counter_(0) {}

    void open()
    {
        if (!reactor()) reactor(ACE_Reactor::instance());
        updateValveState(false);
        ACE_Time_Value delay(1, 0);
        ACE_Time_Value repeat(0, 10000);
        timerId_ = reactor()->schedule_timer(this, 0, delay, repeat);
    }

    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    int close(u_long flags = 0)
    {
        if (timerId_ != -1) reactor()->cancel_timer(timerId_);
        return Task::close(flags);
    }

    int handle_timeout(const ACE_Time_Value& timeout, const void* arg)
    {
        if (isValveOpen()) {
            std::string big(4096, '!');
            Message::Ref ref(Message::Make(big));
            MessageManager mgr(ref);
            put_next(mgr.getMessage());
        }

        return 0;
    }

    bool updateValveState(bool state) { return Task::updateValveState(true); }

    long timerId_;
    int counter_;
};

bool
TestTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    Logger::ProcLog log("deliverDataMessage", Log());
    MessageManager manager(data);

    LOGDEBUG << "*** counter: " << counter_ << " of " << kNumMessages << " ***" << std::endl;

    if (!manager.hasNativeMessageType(MetaTypeInfo::kUnassigned)) {
        LOGERROR << "message does not have expected channel ID" << std::endl;
        ACE_Reactor::instance()->end_reactor_event_loop();
        return false;
    }

    ++counter_;

    if (counter_ == kNumMessages) {
        LOGINFO << "shutting down server" << std::endl;
        ACE_Reactor::instance()->end_reactor_event_loop();
    }

    return true;
}

struct Test : public UnitTest::TestObj {
    Test() : TestObj("SocketChannels") {}
    void test();
    void test1();
    void test2();
};

void
Test::test()
{
    test1();
    ACE_Reactor::instance()->reset_reactor_event_loop();
    test2();
}

void
Test::test1()
{
    Logger::Log::Find("root").setPriorityLimit(Logger::Priority::kDebug);

    Stream stream("Test", 0);
    TModule<ClientSocketWriterTask>* cswtm = new TModule<ClientSocketWriterTask>(&stream);
    stream.push(cswtm);

    ClientSocketWriterTask::Ref cswt(cswtm->getTask());

    // Insert some messages into the stream before we put the rest of the modules.
    //
    std::string big(4096, '!');
    for (int count = 0; count < TestTask::kNumMessages; ++count) {
        Message::Ref ref(Message::Make(big));
        MessageManager mgr(ref);
        stream.put(mgr.getMessage());
    }

    // Now put a counter task that will check messages it gets from up stream.
    //
    TModule<TestTask>* ttm = new TModule<TestTask>(&stream);
    stream.push(ttm);
    ttm->getTask()->open();

    // Finally, push a reader module that opens a service port for the SocketWriterTask to write to.
    //
    TModule<ServerSocketReaderTask>* ssrtm = new TModule<ServerSocketReaderTask>(&stream);
    stream.push(ssrtm);

    assertEqual(true, ssrtm->getTask()->openAndInit("Message"));
    uint16_t port = ssrtm->getTask()->getServerPort();
    assertNotEqual(port, 0);
    assertEqual(true, cswt->openAndInit("Message", "localhost", port));

    // Let 'er rip!
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    assertEqual(int(TestTask::kNumMessages), ttm->getTask()->counter_);

    // Uncomment the following to see the log messages when there are no failures assertFalse(true);
}

void
Test::test2()
{
    Logger::Log::Find("root").setPriorityLimit(Logger::Priority::kDebug);

    // Create a writer stream consisting of a ServerSockerWriter task that will emit messages to connect
    // clients.
    //
    Stream stream("Test", 0);
    TModule<ServerSocketWriterTask>* sswtm = new TModule<ServerSocketWriterTask>(&stream);
    stream.push(sswtm);

    // Open the server port and begin listening for connection attempts
    //
    ServerSocketWriterTask::Ref sswt(sswtm->getTask());
    assertEqual(true, sswt->openAndInit("Message"));
    uint16_t port = sswt->getServerPort();
    assertNotEqual(0, port);

    // Push a counter task that will check messages it gets from up stream.
    //
    TModule<TestTask>* ttm = new TModule<TestTask>(&stream);
    stream.push(ttm);
    ttm->getTask()->open();

    // Add a task to connect to server in the writer stream above.
    //
    TModule<ClientSocketReaderTask>* csrtm = new TModule<ClientSocketReaderTask>(&stream);
    stream.push(csrtm);
    assertEqual(true, csrtm->getTask()->openAndInit("Message", "localhost", port));

    // Let 'er rip!
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    assertEqual(int(TestTask::kNumMessages), ttm->getTask()->counter_);

    // Uncomment the following to see the log messages when there are no failures assertFalse(true);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
