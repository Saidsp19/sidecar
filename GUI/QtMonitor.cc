#include "QtMonitor.h"
#include "Logger/Log.h"

#include "QtCore/QSocketNotifier" // !!! Must be AFTER any boost::signal stuff

using namespace SideCar;
using namespace SideCar::GUI;

Logger::Log&
QtMonitor::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("QtMonitor");
    return log_;
}

QtMonitor::~QtMonitor()
{
    if (socketNotifier_) {
        socketNotifier_->setEnabled(false);
        delete socketNotifier_;
        socketNotifier_ = 0;
    }
}

void
QtMonitor::serviceStarted()
{
    Logger::ProcLog log("serviceStarted", Log());
    LOGINFO << socketNotifier_ << std::endl;

    if (socketNotifier_) {
        delete socketNotifier_;
        socketNotifier_ = 0;
    }

    int socket = getMonitored()->getConnection();
    LOGDEBUG << "socket: " << socket << std::endl;

    socketNotifier_ = new QSocketNotifier(socket, QSocketNotifier::Read, this);

    connect(socketNotifier_, SIGNAL(activated(int)), this, SLOT(processConnection(int)));
}

void
QtMonitor::processConnection(int socket)
{
    socketNotifier_->setEnabled(false);
    getMonitored()->processConnection();
    if (socketNotifier_) socketNotifier_->setEnabled(true);
}

void
QtMonitor::serviceStopping()
{
    Logger::ProcLog log("serviceStopping", Log());
    LOGINFO << socketNotifier_ << std::endl;
    if (socketNotifier_) {
        socketNotifier_->setEnabled(false);
        socketNotifier_->deleteLater();
        socketNotifier_ = 0;
    }
}

QtMonitorFactory::Ref
QtMonitorFactory::Make()
{
    Ref ref(new QtMonitorFactory);
    return ref;
}
