#include "ace/CDR_Stream.h"
#include "ace/FILE_Connector.h"

#include "Logger/Log.h"
#include "Messages/Header.h"
#include "UnitTest/UnitTest.h"
#include "Utils/FilePath.h"

#include "Decoder.h"
#include "MessageManager.h"
#include "Readers.h"
#include "Writers.h"

using namespace SideCar::IO;
using namespace SideCar::Messages;

struct Test : public UnitTest::ProcSuite<Test>
{
    Test() : UnitTest::ProcSuite<Test>(this, "IOTests")
	{
	    add("Reader", &Test::testReader);
	}

    void testReader();
};

class Message : public Header
{
public:
    using Ref = boost::shared_ptr<Message>;

    static const MetaTypeInfo& GetMetaTypeInfo();

    static Ref Make(int one, double two, const std::string& three);

    static Ref Make(ACE_InputCDR& cdr);

    static Header::Ref Loader(ACE_InputCDR& cdr);

    int getOne() const { return one_; }
    double getTwo() const { return two_; }
    const std::string& getThree() const { return three_; }

    ACE_InputCDR& load(ACE_InputCDR& cdr);
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

private:
    Message(int one, double two, const std::string& three)
	: Header("test", GetMetaTypeInfo()), one_(one), two_(two), three_(three) {}

    Message()
	: Header("test", GetMetaTypeInfo()), one_(0), two_(0.0), three_("") {}

    int one_;
    double two_;
    std::string three_;

    static ACE_InputCDR& LoadV1(Message* obj, ACE_InputCDR& cdr);
    static TLoaderRegistry<Message>::VersionedLoader* DefineLoaders();
    static MetaTypeInfo metaTypeInfo_;
    static TLoaderRegistry<Message> loaderRegistry_;
};

TLoaderRegistry<Message>::VersionedLoader*
Message::DefineLoaders()
{
    static TLoaderRegistry<Message>::VersionedLoader loaders_[] = {
	TLoaderRegistry<Message>::VersionedLoader(1, &Message::LoadV1)
    };
    return loaders_;
}

MetaTypeInfo Message::metaTypeInfo_(MetaTypeInfo::Value::kUnassigned, "Message", &Message::Loader, 0);
TLoaderRegistry<Message> Message::loaderRegistry_(Message::DefineLoaders(), 1);

const MetaTypeInfo&
Message::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

Message::Ref
Message::Make(int one, double two, const std::string& three)
{
    Ref ref(new Message(one, two, three));
    return ref;
}

Message::Ref
Message::Make(ACE_InputCDR& cdr)
{
    Ref ref(new Message);
    ref->load(cdr);
    return ref;
}

Header::Ref
Message::Loader(ACE_InputCDR& cdr)
{
    return Make(cdr);
}

ACE_InputCDR&
Message::load(ACE_InputCDR& cdr)
{
    return loaderRegistry_.load(this, cdr);
}

ACE_InputCDR&
Message::LoadV1(Message* obj, ACE_InputCDR& cdr)
{
    obj->Header::load(cdr);
    cdr >> obj->one_;
    cdr >> obj->two_;
    cdr >> obj->three_;
    return cdr;
}

ACE_OutputCDR&
Message::write(ACE_OutputCDR& cdr) const
{
    cdr << loaderRegistry_.getCurrentVersion();
    Header::write(cdr);
    cdr << one_;
    cdr << two_;
    cdr << three_;
    return cdr;
}

void
Test::testReader()
{
    // !!! Uncomment the following to see debugging information if a test case fails.
    //
    // Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    // Create a temporary file to use for testing.
    //
    Utils::TemporaryFilePath fp;
    ACE_FILE_Addr addr(fp);

    std::string big3(1024 * 10, '3');
    std::string big8(2048 * 100, '8');

    {
	FileWriter writer;
	ACE_FILE_Connector fd(writer.getDevice(), addr);
	for (int count = 1; count <= 5; ++count) {
	    Message::Ref msg(Message::Make(count, 2.345, big3));
	    assertTrue(writer.write(MessageManager(msg).getMessage()));
	}

	for (int count = 1; count <= 5; ++count) {
	    Message::Ref msg(Message::Make(count * 2, 7.890, big8));
	    assertTrue(writer.write(MessageManager(msg).getMessage()));
	}
    }

    {
	FileReader reader;
	ACE_FILE_Connector fd(reader.getDevice(), addr);

	// Read in 5 small messages. Tests the Decoder::decode<>() method.
	//
	for (int count = 1; count <= 5; ++count) {
	    std::clog << "big3 count: " << count << '\n';
	    assertFalse(reader.isMessageAvailable());
	    assertTrue(reader.fetchInput());
	    assertTrue(reader.isMessageAvailable());
	    Message::Ref msg(MessageManager(reader.getMessage(),
                                            &Message::GetMetaTypeInfo())
                             .getNative<Message>());
	    assertEqual(count, msg->getOne());
	    assertEqual(2.345, msg->getTwo());
	    assertEqual(big3, msg->getThree());
	}

	// Read in 5 large messages. Tests the Decoder::decodeInto<>() method. Create a dummy message to write
	// into with the decoder.
	//
	Message::Ref msg(Message::Make(-1, -1.0, ""));
	for (int count = 1; count <= 5; ++count) {
	    std::clog << "big8 count: " << count << '\n';
	    assertTrue(reader.fetchInput());
	    assertTrue(reader.isMessageAvailable());
	    Message::Ref msg(MessageManager(reader.getMessage(),
                                            &Message::GetMetaTypeInfo())
                             .getNative<Message>());
	    assertEqual(count * 2, msg->getOne());
	    assertEqual(7.890, msg->getTwo());
	    assertEqual(big8, msg->getThree());
	}
    }

    // !!! Uncomment to see debug messages even if all tests above succeed. assertFalse(true);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
