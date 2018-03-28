#include "ace/Reactor.h"

#include "Logger/Log.h"
#include "Messages/Header.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "Decoder.h"
#include "FileReaderTask.h"
#include "FileWriterTask.h"
#include "MessageManager.h"

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
        Ref ref(new Message);
        ref->load(cdr);
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

    std::ostream& printData(std::ostream& os) const { return os << value_; }

private:
    Message() : Header(GetMetaTypeInfo()), value_("") {}

    Message(const std::string& value) : Header("test", GetMetaTypeInfo(), Header::Ref()), value_(value) {}

    std::string value_;

    static MetaTypeInfo metaTypeInfo_;
};

MetaTypeInfo Message::metaTypeInfo_(MetaTypeInfo::Value::kUnassigned, "Message", &Message::Loader, 0);

const MetaTypeInfo&
Message::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

struct Test : public UnitTest::TestObj {
    Test() : TestObj("FileTasks") {}

    void test();
};

struct TestTask : public Task {
    enum { kMessageCount = 5 };
    TestTask() : Task(), count_(0) { setUsingData(true); }
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);
    bool doShutdownRequest();
    int count_;
};

bool
TestTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    MessageManager mgr(data);
    Message::Ref ref(mgr.getNative<Message>());
    ++count_;
    std::clog << count_ << " *** TestTask::put: " << ref->getValue() << std::endl;
    return true;
}

bool
TestTask::doShutdownRequest()
{
    std::clog << "encountered STOP message\n";
    ACE_Reactor::instance()->end_event_loop();
    return true;
}

void
Test::test()
{
    Logger::Log::Find("root").setPriorityLimit(Logger::Priority::kDebug);

    // Create a temporary file to use for testing.
    //
    Utils::TemporaryFilePath fp("foo");
    FileWriterTask::Ref fwt(FileWriterTask::Make());
    assertTrue(fwt->openAndInit("Message", fp));

    const size_t N = TestTask::kMessageCount;

    for (size_t i = 0; i < N; ++i) {
        std::ostringstream text;
        text << "Message " << i + 1 << " of " << N << std::endl;
        Message::Ref msg = Message::Make(text.str());
        MessageManager mgr(msg);
        assertEqual(0, fwt->put(mgr.getMessage(), 0));
    }

    fwt->close(1);
    assertEqual(N, fwt->getInputStats(0).getMessageCount());

    boost::shared_ptr<TestTask> testTask(new TestTask);
    testTask->setTaskIndex(1);

    FileReaderTask::Ref frt(FileReaderTask::Make());
    frt->setTaskIndex(0);

    Channel channel("foo", "Message");
    channel.setSender(frt);
    channel.addRecipient(testTask, 0);

    frt->addOutputChannel(channel);
    frt->next(testTask.get());
    frt->setUsingData(true);

    assertTrue(frt->openAndInit("Message", fp, true));
    assertTrue(frt->start());

    // Let everything run until the event loop is cancelled by the TestTask::put method above.
    //
    ACE_Reactor::instance()->run_reactor_event_loop();
    frt->close(1);

    assertEqual(N, frt->getInputStats(0).getMessageCount());
    assertEqual(int(TestTask::kMessageCount), testTask->count_);

    frt->next(0);

    // assertTrue(false);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
