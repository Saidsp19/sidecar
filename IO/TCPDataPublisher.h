#ifndef SIDECAR_IO_TCPDATAPUBLISHER_H // -*- C++ -*-
#define SIDECAR_IO_TCPDATAPUBLISHER_H

#include "IO/DataPublisher.h"
#include "IO/Module.h"
#include "IO/ZeroconfRegistry.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Zeroconf {
class Publisher;
}
namespace IO {

class ServerSocketWriterTask;

/** Publisher of data using TCP transport. Relies on a ServerSocketWriterTask to do the heady lifing.
 */
class TCPDataPublisher : public DataPublisher, public ZeroconfTypes::Publisher {
    using Super = DataPublisher;

public:
    using Self = TCPDataPublisher;
    using Ref = boost::shared_ptr<Self>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new TCPDataPublisher objects

        \return reference to new DataPublisher object
    */
    static Ref Make();

    /** Open a server connection on a particular port.

        \param key message type key of published data

        \param serviceName Zeroconf name of service publishing the data

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, const std::string& serviceName, uint16_t port = 0, int bufferSize = 0,
                     long threadFlags = kDefaultThreadFlags, long threadPriority = ACE_DEFAULT_THREAD_PRIORITY);

    /** Override of DataPublisher method. The service is being shutdown.

        \param flags if 1 module is shutting down

        \return 0 if successful, -1 otherwise.
    */
    int close(u_long flags = 0);

    size_t getConnectionCount() const;

protected:
    /** Constructor.
     */
    TCPDataPublisher();

    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout = 0);

    void setServiceName(const std::string& serviceName);

private:
    bool calculateUsingDataValue() const;

    void connectionCountChanged(size_t value);

    using ServerSocketWriterTaskRef = boost::shared_ptr<ServerSocketWriterTask>;
    ServerSocketWriterTaskRef writer_;
};

using TCPDataPublisherModule = TModule<TCPDataPublisher>;

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
