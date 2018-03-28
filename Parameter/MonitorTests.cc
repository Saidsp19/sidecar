#include <iostream>

#include "boost/enable_shared_from_this.hpp"
#include "boost/scoped_ptr.hpp"

#include "Monitor.h"
#include "Parameter.h"

#include "UnitTest/UnitTest.h"

using namespace SideCar;

using MyIntValue = Parameter::TValue<Parameter::Defs::RangedInt<1, 9>>;

class MyIntValueMonitor : public Parameter::Monitor<MyIntValue> {
public:
    MyIntValueMonitor(MyIntValue::Ref& value);

    void valueChanged(const MyIntValue&);

    int getTimesChanged() const { return timesChanged_; }

private:
    int timesChanged_;
};

MyIntValueMonitor::MyIntValueMonitor(MyIntValue::Ref& value) : Parameter::Monitor<MyIntValue>(value), timesChanged_(0)
{
    ;
}

void
MyIntValueMonitor::valueChanged(const MyIntValue& value)
{
    ++timesChanged_;
}

class MonitorTest : public UnitTest::TestObj, public boost::enable_shared_from_this<MonitorTest> {
public:
    using MP = Parameter::MonitorProxy<MyIntValue, MonitorTest>;

    static boost::shared_ptr<MonitorTest> Make()
    {
        boost::shared_ptr<MonitorTest> ref(new MonitorTest);
        return ref;
    }

    void test();

    void value1Changed(const MyIntValue& value);

    void value2Changed(const MyIntValue& value);

private:
    MonitorTest();

    MyIntValue::Ref value1_;
    MyIntValue::Ref value2_;
    MyIntValueMonitor monitor1_;
    MyIntValueMonitor monitor2_;
    boost::scoped_ptr<MP> monitor3_;
    boost::scoped_ptr<MP> monitor4_;
    int times1Changed_;
    int times2Changed_;
};

MonitorTest::MonitorTest() :
    TestObj("Monitor"), value1_(MyIntValue::Make("one", "one test", 1)),
    value2_(MyIntValue::Make("two", "two test", 2)), monitor1_(value1_), monitor2_(value2_), monitor3_(), monitor4_(),
    times1Changed_(0), times2Changed_(0)
{
    ;
}

void
MonitorTest::value1Changed(const MyIntValue&)
{
    ++times1Changed_;
}

void
MonitorTest::value2Changed(const MyIntValue&)
{
    ++times2Changed_;
}

void
MonitorTest::test()
{
    // Create a MonitorProxy that refers to ourselves and our valueChanged method.
    //
    monitor3_.reset(new MP(value1_, shared_from_this(), &MonitorTest::value1Changed));
    monitor4_.reset(new MP(value2_, shared_from_this(), &MonitorTest::value2Changed));

    // Check initial monitor state.
    //
    assertEqual(0, monitor1_.getTimesChanged());
    assertEqual(0, monitor2_.getTimesChanged());
    assertEqual(0, times1Changed_);
    assertEqual(0, times2Changed_);

    // Check monitor state after value change.
    //
    value1_->setValue(2);
    assertEqual(2, value1_->getValue());
    assertEqual(1, monitor1_.getTimesChanged());
    assertEqual(0, monitor2_.getTimesChanged());
    assertEqual(1, times1Changed_);
    assertEqual(0, times2Changed_);

    value2_->setValue(9);
    assertEqual(9, value2_->getValue());
    assertEqual(1, monitor1_.getTimesChanged());
    assertEqual(1, monitor2_.getTimesChanged());
    assertEqual(1, times1Changed_);
    assertEqual(1, times2Changed_);

    // Check invalid value detection from monitor.
    //
    try {
        *value1_ = 10;
        assertTrue(false);
    } catch (Parameter::InvalidValue&) {
        ;
    }
}

int
main(int argc, const char* argv[])
{
    return MonitorTest::Make()->mainRun();
}
