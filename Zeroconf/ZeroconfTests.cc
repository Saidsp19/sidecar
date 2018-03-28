#include <sys/types.h>
#include <unistd.h>

#include "ace/Event_Handler.h"
#include "ace/Reactor.h"
#include "boost/bind.hpp"

#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

#include "Zeroconf/ACEMonitor.h"
#include "Zeroconf/Browser.h"
#include "Zeroconf/Publisher.h"

using namespace SideCar::Zeroconf;

struct Test : public UnitTest::TestObj, public ACE_Event_Handler {
    using ServiceEntryVector = Browser::ServiceEntryVector;

    static Logger::Log& Log()
    {
        static Logger::Log& log = Logger::Log::Find("ZeroconfTests.Test");
        return log;
    }

    Test() :
        TestObj("Zeroconf"), ACE_Event_Handler(ACE_Reactor::instance()), name_(""), port_(::getpid()),
        wasPublished_(false), found_(false)
    {
        char hostName[1024];
        ::gethostname(hostName, 1024);
        std::ostringstream os("");
        os << "ZCT_" << hostName << '_' << ::getpid();
        name_ = os.str();
    }

    void test();

    void publishedNotification(bool state);

    void foundServiceNotification(const ServiceEntryVector& services);

    int handle_timeout(const ACE_Time_Value& duration, const void* arg);

    std::string name_;
    uint16_t port_;
    bool wasPublished_;
    bool found_;
};

void
Test::publishedNotification(bool state)
{
    Logger::ProcLog log("publishedNotification", Log());
    LOGINFO << "state: " << state << std::endl;

    wasPublished_ = state;
    if (state && found_) {
        LOGINFO << "shutting down" << std::endl;
        ACE_Reactor::instance()->end_reactor_event_loop();
    }
}

void
Test::foundServiceNotification(const ServiceEntryVector& services)
{
    Logger::ProcLog log("foundServiceNotification", Log());
    for (size_t index = 0; index < services.size() && !found_; ++index) {
        LOGINFO << "found " << services[index]->getName() << std::endl;
        if (services[index]->getName() == name_) {
            LOGINFO << "found" << std::endl;
            found_ = true;
        }
    }

    if (wasPublished_ && found_) {
        LOGINFO << "shutting down" << std::endl;
        ACE_Reactor::instance()->end_reactor_event_loop();
    }
}

int
Test::handle_timeout(const ACE_Time_Value& duration, const void* arg)
{
    ACE_Reactor::instance()->end_reactor_event_loop();
    return 0;
}

void
Test::test()
{
    Logger::Log::Find("root").setPriorityLimit(Logger::Priority::kDebug);

    ACEMonitorFactory::Ref monitorFactory(ACEMonitorFactory::Make());
    Publisher::Ref publisher(Publisher::Make(monitorFactory->make()));
    publisher->setType("_blah._tcp");
    assertTrue(publisher.unique());
    publisher->connectToPublishedSignal(boost::bind(&Test::publishedNotification, this, _1));

    Browser::Ref browser(Browser::Make(monitorFactory, "_blah._tcp"));
    assertTrue(browser.unique());
    browser->connectToFoundSignal(boost::bind(&Test::foundServiceNotification, this, _1));

    assertTrue(browser->start());
    assertTrue(publisher->setTextData("one", "first", false));
    assertTrue(publisher->setTextData("two", "second"));
    publisher->setPort(port_);
    assertTrue(publisher->publish(name_, false));

    const ACE_Time_Value delay(10);
    const ACE_Time_Value repeat(0);
    long timer = reactor()->schedule_timer(this, 0, delay, repeat);

    ACE_Reactor::instance()->run_reactor_event_loop();

    reactor()->cancel_timer(timer);

    assertTrue(wasPublished_);
    assertTrue(found_);

    publisher->stop();
    browser->stop();
}

int
main(int argc, const char* argv[])
{
    return Test().mainRun();
}
