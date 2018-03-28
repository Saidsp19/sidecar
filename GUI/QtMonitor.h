#ifndef SIDECAR_GUI_QTMONITOR_H // -*- C++ -*-
#define SIDECAR_GUI_QTMONITOR_H

#include "QtCore/QObject"
#include "Zeroconf/Transaction.h" // !!! Must be before any Qt includes !!!

class QSocketNotifier;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

/** Concrete implementation of the Monitor class that relies on QSocketNotifier object to determine when a
    Transaction object has data available for processing. When there is, it invokes
    Transaction::processConnection() to handle the incoming data.
*/
class QtMonitor : public QObject, public Zeroconf::Monitor {
    Q_OBJECT
public:
    static Logger::Log& Log();

    /** Constructor. Just initialize. Wait and create a new QSocketNotifier when there is a socket to monitor.
     */
    QtMonitor() : QObject(), Zeroconf::Monitor(), socketNotifier_(0) {}

    /** Destructor. Dispose of any active QSocketNotifier
     */
    ~QtMonitor();

private slots:

    /** Notification from a QSocketNotifier object that data is available on the monitored socket.

        \param socket internal socket to the DNSSD server (ignored)
    */
    void processConnection(int socket);

private:
    /** Implementation of abstract Monitor method. Notification from the monitored object that a service has
        started. Creates a QSocketNotifier object that takes the internal socket connection to the DNSSD server.
        When it detects that data is available on the socket connection, it sends a signal to the
        processConnection() slot.
    */
    void serviceStarted();

    /** Implementation of abstract Monitor method. Notification from the monitored object that a service is
        stopping. Deletes the QSocketNotifier object created in serviceStarted().
    */
    void serviceStopping();

    QSocketNotifier* socketNotifier_;
};

class QtMonitorFactory : public Zeroconf::MonitorFactory {
public:
    using Ref = boost::shared_ptr<QtMonitorFactory>;

    static Ref Make();

    Zeroconf::Monitor* make() { return new QtMonitor; }
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
