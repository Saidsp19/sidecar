#ifndef SIDECAR_GUI_TCPMESSAGEREADER_H // -*- C++ -*-
#define SIDECAR_GUI_TCPMESSAGEREADER_H

#include "QtNetwork/QTcpSocket"

#include "GUI/MessageReader.h"
#include "IO/Readers.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class TCPSocketReaderDevice {
public:
    TCPSocketReaderDevice() : device_(0) {}

    void setDevice(QTcpSocket* device) { device_ = device; }

    QTcpSocket* getDevice() const { return device_; }

protected:
    ssize_t fetchFromDevice(void* addr, size_t size) { return device_->read(reinterpret_cast<char*>(addr), size); }

private:
    QTcpSocket* device_;
};

class TCPSocketReader : public IO::TReader<IO::StreamReader, TCPSocketReaderDevice> {
    using Super = IO::TReader<IO::StreamReader, TCPSocketReaderDevice>;

public:
    using Ref = boost::shared_ptr<TCPSocketReader>;

    static Ref Make(size_t bufferSize = ACE_DEFAULT_CDR_BUFSIZE)
    {
        Ref ref(new TCPSocketReader(bufferSize));
        return ref;
    }

    TCPSocketReader(size_t bufferSize = ACE_DEFAULT_CDR_BUFSIZE) : Super(bufferSize) {}
};

class TCPMessageReader : public MessageReader {
    Q_OBJECT
    using Super = MessageReader;

public:
    static Logger::Log& Log();

    static TCPMessageReader* Make(const ServiceEntry* service);

    ~TCPMessageReader();

private slots:

    void socketReadyRead();

private:
    TCPMessageReader(const Messages::MetaTypeInfo* metaTypeInfo, QTcpSocket* socket);

    TCPSocketReader reader_;
    QTcpSocket* socket_;
};

} // namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
