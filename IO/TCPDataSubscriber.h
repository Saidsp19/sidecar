#ifndef SIDECAR_IO_TCPDATASUBSCRIBER_H // -*- C++ -*-
#define SIDECAR_IO_TCPDATASUBSCRIBER_H

#include "IO/DataSubscriber.h"
#include "IO/Module.h"
#include "IO/ZeroconfRegistry.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

class TCPConnector;

class TCPDataSubscriber : public DataSubscriber,
			  public ZeroconfTypes::Subscriber
{
    using Super = DataSubscriber;
public:
    using Ref = boost::shared_ptr<TCPDataSubscriber>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new TCPDataSubscriber objects

        \return reference to new TCPDataSubscriber object
    */
    static Ref Make();

    /** Prepare to subscribe to a data publisher with a given service name. Starts a Zeroconf::Browser to watch
	for DataPublisher objects bearing the service name. Only after a DataPublisher object is found and
	resolved is the ClientSocketReaderTask::open() method invoked.

	\param key message type key of data coming in

	\param serviceName Zeroconf name of service publishing the data

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, const std::string& serviceName,
                     int bufferSize = 0, int interface = 0);

    int close(u_long flags = 0);

protected:

    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
	methods.
    */
    TCPDataSubscriber();

    void setServiceName(const std::string& serviceName);

private:

    void resolvedService(const Zeroconf::ServiceEntry::Ref& service);

    void lostService();

    TCPConnector* connector_;
};

using TCPDataSubscriberModule = TModule<TCPDataSubscriber>;

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
