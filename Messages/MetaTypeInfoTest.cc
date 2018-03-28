#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

#include "Header.h"
#include "LoaderRegistry.h"
#include "MetaTypeInfo.h"

using namespace SideCar;
using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() : TestObj("MetaTypeInfo"), called_(false) { testObj_ = this; }

    void test();

    bool called_;

    static Header::Ref Loader1(ACE_InputCDR& cdr)
    {
        testObj_->called_ = true;
        return Header::Ref();
    }

    static Header::Ref Loader2(const std::string& producer, XmlStreamReader& xsr) { return Header::Ref(); }

    static Test* testObj_;
};

Test* Test::testObj_ = 0;

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    {
        MetaTypeInfo m(MetaTypeInfo::Value::kUnassigned, "Special", &Loader1, &Loader2);
        assertEqual(&m, MetaTypeInfo::Find("Special"));
        assertEqual(&m, MetaTypeInfo::Find(MetaTypeInfo::Value::kUnassigned));
        assertTrue(m.isa(MetaTypeInfo::Value::kUnassigned));
        assertEqual(MetaTypeInfo::Value::kUnassigned, m.getKey());
        assertEqual(std::string("Special"), m.getName());

        MetaTypeInfo::SequenceType s1(m.getNextSequenceNumber());
        MetaTypeInfo::SequenceType s2(m.getNextSequenceNumber());
        assertNotEqual(s1, s2);

        try {
            MetaTypeInfo(MetaTypeInfo::Value::kUnassigned, "Special", &Loader1, &Loader2);
            assertTrue(false);
        } catch (const Utils::Exception&) {
            ;
        }

        m.unregister();
    }

    try {
        MetaTypeInfo::Find("Special");
        assertTrue(false);
    } catch (const Utils::Exception&) {
        ;
    }

    try {
        MetaTypeInfo::Find(MetaTypeInfo::Value::kUnassigned);
        assertTrue(false);
    } catch (const Utils::Exception&) {
        ;
    }
}

int
main(int, const char**)
{
    return Test().mainRun();
}
