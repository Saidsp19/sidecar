#include <sstream>

#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"
#include "XMLRPC/XmlRpcValue.h"

#include "Parameter.h"

using namespace SideCar;

class Test : public UnitTest::TestObj
{
public:
    Test() : TestObj("Parameter") {}

    void test();
};

struct DoubleRange
{
    static double GetMinValue() { return -23.45; }
    static double GetMaxValue() { return 67.89; }
};

struct ABCDef : public Parameter::Defs::EnumTypeTraitsBase
{
    enum Foo
    {
	kTwo = 2,
	kThree,
	kFour
    };

    using ValueType = Foo;
    static ValueType GetMinValue() { return kTwo; }
    static ValueType GetMaxValue() { return kFour; }

    static const char* const* GetEnumNames()
	{
	    static const char* names[] = { "A", "B", "C" };
	    return names;
	}
};

struct ABC : Parameter::TValue<Parameter::Defs::Enum<ABCDef>> {};

void
Test::test()
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    ABC::Ref abc(ABC::Make("abc", "abc test", ABC::kTwo));
    assertEqual(2, abc->getValue());

    Parameter::IntValue::Ref foo(Parameter::IntValue::Make("foo", "foo test", 0));
    assertEqual("foo", foo->getName());
    assertEqual(0, foo->getValue());

    Parameter::DoubleValue::Ref bar(Parameter::DoubleValue::Make("bar", "bar test", 0.123));
    assertEqual("bar", bar->getName());
    assertEqual(0.123, bar->getValue());

    Parameter::StringValue::Ref baz(Parameter::StringValue::Make("baz", "baz test", "this is a test"));
    assertEqual("baz", baz->getName());
    assertEqual("this is a test", baz->getValue());

    // Test loading of values from input stream.
    //
    std::istringstream is("123 4.567 'this was a test' \"last test\"");
    is >> *foo >> *bar >> *baz;
    assertEqual(123, foo->getValue());
    assertEqual(4.567, bar->getValue());
    assertEqual("this was a test", baz->getValue());

    // Test again reading a string value but with double quotes.
    //
    is >> *baz;
    assertEqual("last test", baz->getValue());

    // Test saving parameters to an output stream
    //
    std::ostringstream os("");
    os << *foo << ' ' << *bar << ' ' << *baz;
    assertEqual("foo 123 bar 4.567 baz 'last test'", os.str());
    os.str("");

    // Test assignment
    //
    *foo = 987;
    assertEqual(987, foo->getValue());

    // Test integer ranges
    //
    using RIV = Parameter::TValue<Parameter::Defs::RangedInt<10,20>>;
    RIV::Ref riv(RIV::Make("riv", "riv test", 15));
    assertEqual(15, riv->getValue());
    assertFalse(riv->isValid(9));
    assertFalse(riv->isValid(21));
    try {
	riv->setValue(9);
	assertFalse(true);
    }
    catch (const Parameter::InvalidValue& ex) {
	;
    }

    using RDV = Parameter::TValue<Parameter::Defs::RangedDouble<DoubleRange>>;
    RDV::Ref rdv(RDV::Make("rdv", "rdv test", 0.0));
    assertEqual(0.0, rdv->getValue());
    assertTrue(rdv->isValid(-23.45));
    assertTrue(rdv->isValid(67.89));
    try {
	rdv->setValue(-100.0);
	assertFalse(true);
    }
    catch (const Parameter::InvalidValue&) {
	;
    }

    XmlRpc::XmlRpcValue value;

    Parameter::NotificationValue::Ref anot(Parameter::NotificationValue::Make("anot", "anot test", 8));
    anot->describe(value);
    assertEqual(
	"<value><struct>"
	"<member><name>advanced</name><value><boolean>0</boolean></value></member>"
	"<member><name>label</name><value>anot test</value></member>"
	"<member><name>name</name><value>anot</value></member>"
	"<member><name>original</name><value><i4>8</i4></value></member>"
	"<member><name>type</name><value>notification</value></member>"
	"<member><name>value</name><value><i4>8</i4></value></member>"
	"</struct></value>",
	value.toXml());

    abc->describe(value);
    assertEqual(
	"<value><struct>"
	"<member><name>advanced</name><value><boolean>0</boolean></value></member>"
	"<member><name>enumNames</name><value><array><data>"
	"<value>A</value><value>B</value><value>C</value>"
	"</data></array></value></member>"
	"<member><name>label</name><value>abc test</value></member>"
	"<member><name>max</name><value><i4>4</i4></value></member>"
	"<member><name>min</name><value><i4>2</i4></value></member>"
	"<member><name>name</name><value>abc</value></member>"
	"<member><name>original</name><value><i4>2</i4></value></member>"
	"<member><name>type</name><value>enum</value></member>"
	"<member><name>value</name><value><i4>2</i4></value></member>"
	"</struct></value>",
	value.toXml());

    foo->describe(value);
    assertEqual(
	"<value><struct>"
	"<member><name>advanced</name><value><boolean>0</boolean></value></member>"
	"<member><name>label</name><value>foo test</value></member>"
	"<member><name>name</name><value>foo</value></member>"
	"<member><name>original</name><value><i4>0</i4></value></member>"
	"<member><name>type</name><value>int</value></member>"
	"<member><name>value</name><value><i4>987</i4></value></member>"
	"</struct></value>",
	value.toXml());

    bar->describe(value);
    assertEqual(
	"<value><struct>"
	"<member><name>advanced</name><value><boolean>0</boolean></value></member>"
	"<member><name>label</name><value>bar test</value></member>"
	"<member><name>name</name><value>bar</value></member>"
	"<member><name>original</name><value><double>0.123000</double></value></member>"
	"<member><name>type</name><value>double</value></member>"
	"<member><name>value</name><value><double>4.567000</double></value></member>"
	"</struct></value>",
	value.toXml());

    baz->describe(value);
    assertEqual(
	"<value><struct>"
	"<member><name>advanced</name><value><boolean>0</boolean></value></member>"
	"<member><name>label</name><value>baz test</value></member>"
	"<member><name>name</name><value>baz</value></member>"
	"<member><name>original</name><value>this is a test</value></member>"
	"<member><name>type</name><value>string</value></member>"
	"<member><name>value</name><value>last test</value></member>"
	"</struct></value>",
	value.toXml());

    Parameter::Ref ref(riv);
    ref->describe(value);
    assertEqual(
	"<value><struct>"
	"<member><name>advanced</name><value><boolean>0</boolean></value></member>"
	"<member><name>label</name><value>riv test</value></member>"
	"<member><name>max</name><value><i4>20</i4></value></member>"
	"<member><name>min</name><value><i4>10</i4></value></member>"
	"<member><name>name</name><value>riv</value></member>"
	"<member><name>original</name><value><i4>15</i4></value></member>"
	"<member><name>type</name><value>int</value></member>"
	"<member><name>value</name><value><i4>15</i4></value></member>"
	"</struct></value>",
	value.toXml());

    ref = rdv;
    ref->describe(value);
    assertEqual(
	"<value><struct>"
	"<member><name>advanced</name><value><boolean>0</boolean></value></member>"
	"<member><name>label</name><value>rdv test</value></member>"
	"<member><name>max</name><value><double>67.890000</double>"
	"</value></member>"
	"<member><name>min</name><value><double>-23.450000</double>"
	"</value></member>"
	"<member><name>name</name><value>rdv</value></member>"
	"<member><name>original</name><value><double>0.000000</double></value></member>"
	"<member><name>type</name><value>double</value></member>"
	"<member><name>value</name><value><double>0.000000</double>"
	"</value></member></struct></value>",
	value.toXml());

    using DRI = Parameter::TValue<Parameter::Defs::DynamicRangedTypeTraits<Parameter::Defs::IntTypeTraits>>;
    DRI::Ref dri(DRI::Make("dri", "dri test"));
    dri->setMinValue(1);
    dri->setMaxValue(10);
    dri->setValue(3);
    try {
	dri->setValue(11);
	assertFalse(true);
    }
    catch (const Parameter::InvalidValue& ex) {
	;
    }

    try {
	dri->setValue(-3);
	assertFalse(true);
    }
    catch (const Parameter::InvalidValue& ex) {
	;
    }

    ref = dri;
    ref->describe(value);
    assertEqual(
	"<value><struct>"
	"<member><name>advanced</name><value><boolean>0</boolean></value></member>"
	"<member><name>label</name><value>dri test</value></member>"
	"<member><name>max</name><value><i4>10</i4>"
	"</value></member>"
	"<member><name>min</name><value><i4>1</i4>"
	"</value></member>"
	"<member><name>name</name><value>dri</value></member>"
	"<member><name>original</name><value><i4>0</i4></value></member>"
	"<member><name>type</name><value>int</value></member>"
	"<member><name>value</name><value><i4>3</i4>"
	"</value></member></struct></value>",
	value.toXml());
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
