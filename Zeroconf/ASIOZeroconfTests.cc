#include <sys/types.h>
#include <unistd.h>

#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"

#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

#include "Zeroconf/ASIOMonitor.h"
#include "Zeroconf/Browser.h"
#include "Zeroconf/Publisher.h"

using namespace SideCar::Zeroconf;

struct Test : public UnitTest::TestObj, public boost::enable_shared_from_this<Test> {
    using Ptr = boost::shared_ptr<Test>;
    using ServiceEntryVector = Browser::ServiceEntryVector;

    static Logger::Log& Log()
    {
        static Logger::Log& log = Logger::Log::Find("ZeroconfTests.Test");
        return log;
    }

    Test(boost::asio::io_service& ios) :
        TestObj("Zeroconf"), ios_(ios), name_(""), port_(::getpid()), wasPublished_(false), found_(false), timer_(ios)
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

    void handleTimeout();

    boost::asio::io_service& ios_;
    std::string name_;
    uint16_t port_;
    bool wasPublished_;
    bool found_;
    boost::asio::deadline_timer timer_;
};

void
Test::publishedNotification(bool state)
{
    Logger::ProcLog log("publishedNotification", Log());
    LOGINFO << "state: " << state << std::endl;

    wasPublished_ = state;
    if (state && found_) {
        LOGINFO << "shutting down" << std::endl;
        timer_.cancel();
        ios_.stop();
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
        timer_.cancel();
        ios_.stop();
    }
}

void
Test::handleTimeout()
{
    Logger::ProcLog log("handleTimeout", Log());
    LOGERROR << "got timeout" << std::endl;
    ios_.stop();
}

void
Test::test()
{
    Logger::Log::Find("root").setPriorityLimit(Logger::Priority::kDebug);

    timer_.expires_from_now(boost::posix_time::seconds(10));
    timer_.async_wait(boost::bind(&Test::handleTimeout, shared_from_this()));

    ASIOMonitorFactory::Ref monitorFactory(ASIOMonitorFactory::Make(ios_));
    Publisher::Ref publisher(Publisher::Make(monitorFactory->make()));

    publisher->setType("_blah._tcp");
    assertTrue(publisher.unique());

    publisher->connectToPublishedSignal(boost::bind(&Test::publishedNotification, this, _1));

    assertTrue(publisher->setTextData("one", "first", false));
    assertTrue(publisher->setTextData("two", "second"));
    publisher->setPort(port_);
    assertTrue(publisher->publish(name_, false));

    Browser::Ref browser(Browser::Make(monitorFactory, "_blah._tcp"));
    assertTrue(browser.unique());

    browser->connectToFoundSignal(boost::bind(&Test::foundServiceNotification, this, _1));

    assertTrue(browser->start());

    ios_.run();

    assertTrue(wasPublished_);
    assertTrue(found_);

    publisher->stop();
    browser->stop();
}

int
main(int argc, const char* argv[])
{
    boost::asio::io_service ios;
    Test::Ptr test(new Test(ios));
    return test->mainRun();
}
