#include "QtNetwork/QTcpSocket"

#include "GUI/LogUtils.h"

#include "RecordingInfo.h"
#include "RemoteClient.h"

using namespace SideCar::GUI::Master;

Logger::Log&
RemoteClient::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.RemoteClient");
    return log_;
}

RemoteClient::RemoteClient(RecordingController& parent, QTcpSocket& socket) :
    Super(&parent), parent_(parent), socket_(socket), buffer_()
{
    Logger::ProcLog log("RemoteClient", Log());
    LOGINFO << std::endl;
    connect(&socket, SIGNAL(readyRead()), SLOT(readyRead()));
    connect(&socket, SIGNAL(disconnected()), SLOT(close()));
}

void
RemoteClient::readyRead()
{
    Logger::ProcLog log("readyRead", Log());
    LOGINFO << std::endl;

    QByteArray data = socket_.readAll();
    if (data.size() == 0) {
        LOGDEBUG << "EOF - closing client" << std::endl;
        close();
        return;
    }

    buffer_.append(data);
    LOGDEBUG << "data: " << buffer_.data() << std::endl;
    int index = buffer_.indexOf('\n');
    if (index == -1) return;

    if (index < 1) {
        buffer_ = buffer_.mid(index + 1);
        return;
    }

    QByteArray command(buffer_.mid(0, index));
    LOGDEBUG << "command: |" << command.data() << '|' << std::endl;
    buffer_ = buffer_.mid(index + 1);
    LOGDEBUG << "new buffer: |" << buffer_.data() << '|' << std::endl;

    QByteArray response;
    if (command == "start") {
        RecordingController::Status status = parent_.start();
        setError(response, status);
        if (status == RecordingController::kOK) {
            response.append(' ');
            response.append(parent_.getActiveRecording()->getName());
        }
    } else if (command == "stop") {
        setError(response, parent_.stop());
    } else {
        response.append("*** Unkown command: '");
        response.append(command);
        response.append("'");
    }

    LOGDEBUG << "response: " << response.data() << std::endl;
    response.append('\n');
    socket_.write(response);
}

void
RemoteClient::close()
{
    Logger::ProcLog log("close", Log());
    LOGINFO << std::endl;
    socket_.deleteLater();
    deleteLater();
}

void
RemoteClient::setError(QByteArray& buffer, RecordingController::Status status)
{
    switch (status) {
    case RecordingController::kOK: buffer.append("OK"); break;

    case RecordingController::kNoLoadedConfig: buffer.append("*** No configuration file loaded in Master"); break;

    case RecordingController::kFailedCreateDirectory:
        buffer.append("*** Failed to create new recording directory");
        break;

    case RecordingController::kFailedRecordingSetup:
        buffer.append("*** Failed to initialize new recording directory");
        break;

    case RecordingController::kFailedPostRecordingStateChange:
        buffer.append("*** Failed to command runners to new recording state");
        break;
    }
}
