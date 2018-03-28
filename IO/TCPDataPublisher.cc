#include "Logger/Log.h"

#include "ServerSocketWriterTask.h"
#include "TCPDataPublisher.h"

using namespace SideCar::IO;
using namespace SideCar::Zeroconf;

Logger::Log&
TCPDataPublisher::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.TCPDataPublisher");
    return log_;
}

TCPDataPublisher::Ref
TCPDataPublisher::Make()
{
    Ref ref(new TCPDataPublisher);
    return ref;
}

TCPDataPublisher::TCPDataPublisher() : Super(), writer_(ServerSocketWriterTask::Make())
{
    writer_->connectConnectionCountChangedTo(boost::bind(&TCPDataPublisher::connectionCountChanged, this, _1));
}

void
TCPDataPublisher::setServiceName(const std::string& serviceName)
{
    Super::setServiceName(serviceName);
    std::ostringstream os;
    os << serviceName << " PUB";
    setTaskName(os.str());
    writer_->setTaskName(os.str());
}

bool
TCPDataPublisher::openAndInit(const std::string& key, const std::string& serviceName, uint16_t port, int bufferSize,
                              long threadFlags, long threadPriority)
{
    static Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "key: " << key << " serviceName: " << serviceName << " port: " << port << " bufferSize: " << bufferSize
            << " threadFlags: " << std::hex << threadFlags << std::dec << " threadPriority: " << threadPriority
            << std::endl;

    setMetaTypeInfoKeyName(key);
    setTransport("tcp");

    if (!writer_->openAndInit(key, port, bufferSize, threadFlags, threadPriority)) {
        LOGDEBUG << "failed ServerSocketWriterTask::open()" << std::endl;
        setError("Failed to open connection");
        return false;
    }

    std::string serviceType(MakeZeroconfType(key));
    LOGDEBUG << "serviceType: " << serviceType << std::endl;
    setType(serviceType);

    port = writer_->getServerPort();
    setPort(port);

    if (!publish(serviceName)) {
        LOGERROR << "failed to publish connection with name '" << serviceName << " type: " << serviceType << std::endl;
        setError("Failed to publish connection info.");
        return false;
    }

    std::ostringstream os;
    os << "Port: " << port;
    setConnectionInfo(os.str());

    return true;
}

int
TCPDataPublisher::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << "flags: " << flags << std::endl;

    setConnectionInfo("");
    writer_->close(flags);
    return Super::close(flags);
}

bool
TCPDataPublisher::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return writer_->injectDataMessage(data);
}

size_t
TCPDataPublisher::getConnectionCount() const
{
    return writer_->getConnectionCount();
}

void
TCPDataPublisher::connectionCountChanged(size_t count)
{
    Logger::ProcLog log("connectionCountChanged", Log());
    LOGINFO << count << std::endl;
    updateUsingDataValue();
}

bool
TCPDataPublisher::calculateUsingDataValue() const
{
    return getConnectionCount() > 0 || Super::calculateUsingDataValue();
}
