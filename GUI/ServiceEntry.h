#ifndef SIDECAR_GUI_SERVICEENTRY_H // -*- C++ -*-
#define SIDECAR_GUI_SERVICEENTRY_H

#include <inttypes.h>

#include "boost/shared_ptr.hpp"

#include "QtCore/QObject"
#include "QtCore/QMetaType"

namespace Logger { class Log; }

namespace SideCar {
namespace Messages { class MetaTypeInfo; }
namespace Zeroconf { class ServiceEntry; class ResolvedEntry; }
namespace GUI {

/** Bridge class that brings together the Zeroconf::ServiceEntry interface and the Qt QObject interface. A
    ServiceEntry may be queried for connection information, and whether its connection information has been
    resolved to a host/port. If it has not, calling its resolve() method will do so.
*/
class ServiceEntry : public QObject
{
    Q_OBJECT
public:

    using ZCServiceEntryRef = boost::shared_ptr<Zeroconf::ServiceEntry>;

    /** Obtain the log device for ServiceEntry objects.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.

        \param parent owner of the ServiceEntry object

        \param info internal object holding that holds the actual service
        information.
    */
    ServiceEntry(QObject* parent, const Messages::MetaTypeInfo* metaTypeInfo,
                 const ZCServiceEntryRef& service);

    /** Destructor. Defined here so that we may keep ServiceInfo undefined until ServiceBrowser.cc.
     */
    ~ServiceEntry();

    /** Create a duplicate of this object, including any cached connection information.

        \return new ServiceEntry object
    */
    ServiceEntry* duplicate() const;

    /** Obtain a reference to the original Zeroconf::ServiceEntry object

        \return Zeroconf ServiceEntry reference
    */
    const ZCServiceEntryRef& getZeroconfServiceEntry() const
	{ return zeroconfServiceEntry_; }

    /** Obtain the Zeroconf::ResolvedEntry object for a resolved Zeroconf::ServiceEntry object. NOTE: only valid
        if isResolved() returns true.

        \return Zeroconf::ResolvedEntry reference
    */
    const Zeroconf::ResolvedEntry& getZeroconfResolvedEntry() const;

    /** Obtain the sub-type of the service (eg. 'Video' or 'Extractions')

        \return service sub-type
    */
    const Messages::MetaTypeInfo* getMetaTypeInfo() const
	{ return metaTypeInfo_; }

    /** Obtain the name of the service.

        \return service name
    */
    const QString& getName() const { return name_; }

    /** Obtain the type of the service (eg. '_sidecar._tcp')

        \return service type
    */
    const QString& getType() const { return type_; }

    /** Obtain the domain of the service (eg. 'local.')

        \return service domain
    */
    const QString& getDomain() const { return domain_; }
    
    /** Obtain the full name of the service. This is the name + domain.

        \return full service name
    */
    const QString& getFullName() const { return fullName_; }

    /** 

        \return 
    */
    const QString& getNativeHost() const { return nativeHost_; }

    /** Obtain the host on which the service is running. Only valid if the service has been resolved.

        \return service host name
    */
    const QString& getHost() const { return host_; }
    
    /** 

        \return 
    */
    const QString& getTransport() const { return transport_; }

    /** Obtain the interface on which the service may be contacted. Only valid if the service has been resolved.

        \return interface ID
    */
    uint32_t getInterface() const;
    
    /** Obtain the port on the host getHost() over which the service may be contacted .

        \return 
    */
    uint16_t getPort() const;

    /** Determine whether the service host/port information has been previously resolved.

        \return true if so
    */
    bool isResolved() const;

    /** Ask to service to resolve its host/port information.

        \return true if successful, false otherwise.
    */
    bool resolve();

    /** Obtain the value in a TXT record for a given key.

        \param key the value to search for

        \return true if key found, false otherwise
    */
    QString getTextEntry(const QString& key) const;

    /** 

        \param key 

        \param value 

        \return 
    */
    bool hasTextEntry(const QString& key, QString* value = 0) const;

signals:

    /** Signal sent out when the service entry connection information has been resolved

        \param serivceEntry the service that was resolved
    */
    void resolved(ServiceEntry* serivceEntry);

private:

    void resolvedNotification(const ZCServiceEntryRef& service);

    const Messages::MetaTypeInfo* metaTypeInfo_;
    ZCServiceEntryRef zeroconfServiceEntry_;
    QString name_;
    QString type_;
    QString domain_;

    /** NOTE: the following attributes are valid only when isResolved() returns true.
     */
    
    QString fullName_;
    QString nativeHost_;
    QString host_;
    QString transport_;
    struct Private;
    Private* p_;
};

} // end namespace GUI
} // end namespace SideCar

Q_DECLARE_METATYPE(SideCar::GUI::ServiceEntry*)

/** \file
 */

#endif
