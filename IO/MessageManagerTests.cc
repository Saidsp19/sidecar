#include "ace/Message_Block.h"

#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

#include "MessageManager.h"

using namespace SideCar;
using namespace SideCar::IO;
using namespace SideCar::Messages;

class Message : public Header {
public:
    using Ref = boost::shared_ptr<Message>;

    static const MetaTypeInfo& GetMetaTypeInfo();

    static Ref Make(const std::string& value)
    {
        Ref ref(new Message(value));
        return ref;
    }

    static Header::Ref Loader(ACE_InputCDR& cdr)
    {
        Ref ref(new Message(cdr));
        return ref;
    }

    const std::string& getValue() const { return value_; }

    ACE_InputCDR& load(ACE_InputCDR& cdr)
    {
        cdr >> value_;
        return cdr;
    }

    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const
    {
        cdr << value_;
        return cdr;
    }

private:
    Message(ACE_InputCDR& cdr) : Header(GetMetaTypeInfo()), value_("") { load(cdr); }

    Message(const std::string& value) : Header("test", GetMetaTypeInfo()), value_(value) {}

    std::string value_;

    static MetaTypeInfo metaTypeInfo_;
};

MetaTypeInfo Message::metaTypeInfo_(MetaTypeInfo::Value::kUnassigned, "Invalid", &Message::Loader, 0);

const MetaTypeInfo&
Message::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

struct Test : public UnitTest::TestObj {
    Test() : TestObj("MessageManager") {}

    void test();
};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    std::string big(1024 * 16, '!');
    Message::Ref ref(Message::Make(big));
    assertEqual(1, ref.use_count());

    MessageManager mm(ref);

    assertEqual(2, ref.use_count());
    assertTrue(mm.hasNativeMessageType(Message::GetMetaTypeInfo().getKey()));
    assertTrue(mm.hasNative());
    assertEqual(MessageManager::kMetaData, mm.getMessageType());

    ACE_Message_Block* encoded = mm.getEncoded();
    assertEqual(2, ref.use_count());
    assertEqual(2, encoded->data_block()->reference_count());

    ACE_Message_Block* another = mm.getEncoded();
    assertEqual(2, ref.use_count());
    assertEqual(3, another->data_block()->reference_count());

    assertEqual(encoded->data_block(), another->data_block());

    encoded->release();
    assertEqual(2, another->data_block()->reference_count());
    another->release();

    assertEqual(2, ref.use_count());

    {
        MessageManager mm2(ref);
        assertEqual(3, ref.use_count());
        assertTrue(mm2.hasNative());
        assertFalse(mm2.hasEncoded());
    }

    assertEqual(2, ref.use_count());

    std::vector<ACE_Message_Block*> blocks_;
    for (int count = 0; count < 5000; ++count) { blocks_.push_back(MessageManager::MakeMessageBlock(1024)); }

    for (int count = 0; count < 5000; ++count) { blocks_[count]->release(); }

    // !!! Uncomment to see debug messages even if all tests above succeed. assertFalse(true);
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
