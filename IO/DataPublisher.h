#ifndef SIDECAR_IO_DATAPUBLISHER_H // -*- C++ -*-
#define SIDECAR_IO_DATAPUBLISHER_H

#include "IO/IOTask.h"
#include "Zeroconf/Publisher.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** Abstract base class for data publishers. Manages a Zeroconf::Publisher object that publishes connection
    information for data subscribers to use to connect to the data service provided by derived classes.

    Note that this class is still abstract since it does not define the IO::Task::deliverDataMessage() method.
*/
class DataPublisher : public IOTask {
    using Super = IOTask;

public:
    using Self = DataPublisher;
    using Ref = boost::shared_ptr<Self>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Destructor. Shuts down the publisher if it is still running.
     */
    ~DataPublisher();

    void setTransport(const std::string& transport)
    {
        connectionPublisher_->setTextData("transport", transport, false);
    }

    void setIP(const std::string& ip) { connectionPublisher_->setTextData("ip", ip, false); }

    /** Set the Zeroconf type used by the publisher. See ZeroconfRegistry.h for a list of valid types used by
        the SideCar system.

        \param type Zeroconf type
    */
    void setType(const std::string& type) { connectionPublisher_->setType(type); }

    void setHost(const std::string& host) { connectionPublisher_->setHost(host); }

    void setPort(uint16_t port) { connectionPublisher_->setPort(port); }

    void setInterface(uint32_t interface) { connectionPublisher_->setInterface(interface); }

    std::string getType() const { return connectionPublisher_->getType(); }

    uint16_t getPort() const { return connectionPublisher_->getPort(); }

    uint32_t getInterface() const { return connectionPublisher_->getInterface(); }

    bool isPublished() const { return connectionPublisher_->isPublished(); }

    /** Begin publishing connection information under a specific name.

        \param serviceName Zeroconf name of service publishing the data

        \return true if successful, false otherwise
    */
    bool publish(const std::string& serviceName);

    /** Override of IOTask. The service is being shutdown. Stops the Zeroconf::Publisher.

        \param flags if 1 module is shutting down

        \return 0 if successful, -1 otherwise.
    */
    int close(u_long flags = 0) override;

    /** Obtain reference to the Zeroconf::Publisher used by this object.

        \return Zeroconf::Publisher::Ref value
    */
    Zeroconf::Publisher::Ref getConnectionPublisher() const { return connectionPublisher_; }

    const std::string& getServiceName() const { return serviceName_; }

protected:
    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
        methods.
    */
    DataPublisher();

    virtual void publishCallback(bool state);

    virtual void publishSucceeded();

    virtual void publishFailed();

    virtual void setServiceName(const std::string& serviceName);

    /** Callback invoked when internal timer fires. Attempt publish service availability via Zeroconf.

        \param duration the timer period

        \param arg the timer that fired

        \return 0 always
    */
    int handle_timeout(const ACE_Time_Value& duration, const void* arg) override;

private:
    Zeroconf::Publisher::Ref connectionPublisher_;
    Zeroconf::Publisher::SignalConnection connection_;
    std::string serviceName_;
    long timer_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
