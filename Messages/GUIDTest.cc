#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

#include "BinaryVideo.h"
#include "GUID.h"
#include "MetaTypeInfo.h"

using namespace SideCar;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj
{
    Test() : TestObj("GUID") {}

    void test();
};

void
Test::test()
{
    Logger::Log& log(Logger::Log::Root());
    log.setPriorityLimit(Logger::Priority::kDebug1);

    const MetaTypeInfo& ti(BinaryVideo::GetMetaTypeInfo());
    GUID g1("hello", ti);
    assertEqual(std::string("hello"), g1.getProducerName());
    assertEqual(MetaTypeInfo::Value::kBinaryVideo, g1.getMessageTypeKey());
    assertEqual(1U, g1.getMessageSequenceNumber());

    std::string g1rep(g1.getRepresentation());
    log.debug1() << g1rep << std::endl;

    // Create a second GUID, and check that the resulting representations are the same except for the end
    // message sequence counter.
    //
    GUID g2("hello", ti);
    std::string g2rep(g2.getRepresentation());
    log.debug1() << g2rep << std::endl;

    assertNotEqual(g1rep, g2rep);
    assertEqual(g1rep.substr(0, g1rep.size() - 1),
                g2rep.substr(0, g2rep.size() - 1));
}

int
main(int, const char**)
{
    return Test().mainRun();
}
