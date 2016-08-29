#ifndef SIDECAR_GUI_SERVICEBROWSER_H // -*- C++ -*-
#define SIDECAR_GUI_SERVICEBROWSER_H

#include <vector>

#include "boost/shared_ptr.hpp"

#include "QtCore/QHash"
#include "QtCore/QObject"

#include "GUI/QtMonitor.h"
#include "Messages/MetaTypeInfo.h"

namespace Logger { class Log; }

namespace SideCar {
namespace Zeroconf { class Browser; class ServiceEntry; class ResolvedEntry; }
namespace GUI {

class ServiceEntry;
using ServiceEntryHash = QHash<QString,ServiceEntry*>;
using ServiceEntryList = QList<ServiceEntry*>;

/** A refined Zeroconf browser for use by Qt widgets. Keeps track of the active services, represented by
    ServiceEntry objects. Users can obtain found ServiceEntry objects by name.
*/
class ServiceBrowser : public QObject
{
    Q_OBJECT
public:

    /** The following typedefs appear here so that we do not have to include Zeroconf/Browser.h, which also
	includes boost/signal.hpp, which has problems with QT includes.
    */
    using ZCBrowserRef = boost::shared_ptr<Zeroconf::Browser>;
    using ZCServiceEntryRef = boost::shared_ptr<Zeroconf::ServiceEntry>;
    using ZCResolvedEntryRef = boost::shared_ptr<Zeroconf::ResolvedEntry>;
    using ZCServiceEntryVector = std::vector<ZCServiceEntryRef>;

    /** Log device used by ServiceBrowser objects.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.

        \param parent owner relationship used for auto-destruction

        \param serviceType a Zeroconf type designator that determines which
        services the browser sees.
    */
    ServiceBrowser(QObject* parent, const QString& type);

    /** Start the browser.
     */
    void start();

    /** Stop the browser.
     */
    void stop();
    
    /** Obtain a ServiceEntry object under a given name.

        \param name the name of the ServiceEntry to look for

        \return object found, or NULL if none
    */
    ServiceEntry* getServiceEntry(const QString& name) const;

    const QString& getType() const { return type_; }

signals:
    
    /** Notification sent out when the set of active services changes.

        \param found collection of active services
    */
    void availableServices(const ServiceEntryHash& found);
    
    /** Notification sent out when there are new services.

        \param found collection of new services
    */
    void foundServices(const ServiceEntryList& found);

    /** Notification sent out when services disappear.

        \param lost collection of lost services
    */
    void lostServices(const ServiceEntryList& lost);

private:

    /** Notification handler for the Zeroconf::Browser::found() signal.

        \param browser the object sending the found() signal

        \param service Zeroconf::ServiceEntry reference

        \param moreToCome if true, there are more services to follow
    */
    void foundNotification(const ZCServiceEntryVector& services);

    /** Notification handler for the Zeroconf::Browser::lost() signal.

        \param browser the object sending the lost() signal

        \param service Zeroconf::ServiceEntry reference

        \param moreToCome if true, there are more services to follow
    */
    void lostNotification(const ZCServiceEntryVector& services);

    ZCBrowserRef browser_;
    ServiceEntryHash active_;
    QString type_;
    const Messages::MetaTypeInfo* metaTypeInfo_;
    static QtMonitorFactory::Ref monitorFactory_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
