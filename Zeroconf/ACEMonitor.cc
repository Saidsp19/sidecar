#include "ace/Reactor.h"

#include "Logger/Log.h"

#include "ACEMonitor.h"
#include "Transaction.h"

using namespace SideCar::Zeroconf;

Logger::Log&
ACEMonitor::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Zeroconf.ACEMonitor");
    return log_;
}

ACEMonitor::ACEMonitor() : ACE_Event_Handler(ACE_Reactor::instance()), Monitor()
{
    Logger::ProcLog log("ACEMonitor", Log());
    LOGINFO << this << std::endl;
    ;
}

ACEMonitor::~ACEMonitor()
{
    Logger::ProcLog log("~ACEMonitor", Log());
    LOGINFO << this << std::endl;
    ;
}

void
ACEMonitor::serviceStarted()
{
    Logger::ProcLog log("serviceStarted", Log());
    LOGINFO << this << std::endl;
    ;
    if (reactor()->register_handler(this, ACE_Event_Handler::READ_MASK) == -1) {
        LOGERROR << "failed ACE_Reactor::register_handler" << std::endl;
    }
}

void
ACEMonitor::serviceStopping()
{
    Logger::ProcLog log("serviceStopping", Log());
    LOGINFO << this << std::endl;
    ;
    if (reactor()->remove_handler(this, ACE_Event_Handler::READ_MASK) == -1) {
        LOGERROR << "failed ACE_Reactor::remove_handler" << std::endl;
    }
}

ACE_HANDLE
ACEMonitor::get_handle() const
{
    return getMonitored()->getConnection();
}

int
ACEMonitor::handle_input(ACE_HANDLE fd)
{
    Logger::ProcLog log("handleInput", Log());
    LOGDEBUG << this << std::endl;
    ;
    return getMonitored()->processConnection() ? 0 : -1;
}

ACEMonitorFactory::Ref
ACEMonitorFactory::Make()
{
    Ref ref(new ACEMonitorFactory);
    return ref;
}
