#include "ace/Reactor.h"

#include "IO/Module.h"
#include "IO/Stream.h"
#include "Logger/Log.h"
#include "Messages/Header.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "FileReaderTask.h"
#include "FileWriterTask.h"
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
	: Header("test", GetMetaTypeInfo(), Header::Ref()), value_(value) {}

    std::string value_;

    static MetaTypeInfo metaTypeInfo_;
};

MetaTypeInfo Message::metaTypeInfo_(MetaTypeInfo::Value::kUnassigned, "Message", &Message::Loader, 0);

const MetaTypeInfo&
Message::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

struct TestTask : public Task
{
    enum { kNumMessages = 5 };
    using Ref = boost::shared_ptr<TestTask>;
    static Ref Make() { Ref ref(new TestTask); return ref; }
    TestTask() : Task(), count_(0) {}
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);
    bool doShutdownRequest();
    int count_;
};

bool
TestTask::doShutdownRequest()
{
    ACE_Reactor::instance()->end_reactor_event_loop();
    return true;
}

bool
TestTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    static Logger::Log& log =
	Logger::Log::Find("TestTask::deliverDataMessage");

    MessageManager manager(data);

    LOGDEBUG << "*** TestTask::put: " << count_ << " Type: "
	     << manager.getMessageType()
	     << std::endl;

    if (! manager.hasNativeMessageType(Messages::MetaTypeInfo::Value::kUnassigned)) {
	LOGERROR << "message does not have expected channel ID" << std::endl;
	ACE_Reactor::instance()->end_reactor_event_loop();
	return false;
    }

    ++count_;

    return true;
}

struct Test : public UnitTest::TestObj
{
    Test() : TestObj("FileModules") {}
    void test();
};

void
Test::test()
{
    Logger::Log::Find("root").setPriorityLimit(Logger::Priority::kDebug);

    Stream::Ref stream(Stream::Make("Stream1"));

    TModule<FileWriterTask>* fwtm = new TModule<FileWriterTask>(stream);
    stream->push(fwtm);

    FileWriterTask::Ref fwt(fwtm->getTask());
    Utils::TemporaryFilePath fp("fileModuleTests.tmp");
    assertTrue(fwt->openAndInit("Message", fp));

    const int N = TestTask::kNumMessages;
    for (int i = 0; i < N; ++i) {
        std::ostringstream text;
        text << "Message " << i+1 << " of " << N << std::endl;
        MessageManager mgr(Message::Make(text.str()));
        assertEqual(0, fwt->put(mgr.getMessage(), 0));
    }

    // Now close and delete the stream (and the contained tasks)
    //
    stream->close();

    // Create a new stream and add two modules: a message counter and a FileReaderTask.
    //
    stream = Stream::Make("Stream2");
    TModule<TestTask>* ttm = new TModule<TestTask>(stream);
    stream->push(ttm);

    TModule<FileReaderTask>* frtm = new TModule<FileReaderTask>(stream);
    stream->push(frtm);
    assertTrue(frtm->getTask()->openAndInit("Message", fp, true));
    assertTrue(frtm->getTask()->start());

    // Let everything run until the event loop is cancelled by the TestTask::put method above.
    //
    ACE_Reactor::instance()->run_reactor_event_loop();

    assertEqual(int(TestTask::kNumMessages), ttm->getTask()->count_);

    stream->close();

    // assertTrue(false);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
