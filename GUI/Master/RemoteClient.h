#ifndef SIDECAR_GUI_MASTER_REMOTECLIENT_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_REMOTECLIENT_H

#include "QtCore/QByteArray"
#include "QtCore/QObject"

#include "RecordingController.h"

class QTcpSocket;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Master {

/** TCP command interface that allows control of the recordings via command-line utility.
 */
class RemoteClient : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    /** Log device to use for RemoteClient log messages.

        \return Log device
    */
    static Logger::Log& Log();

    RemoteClient(RecordingController& parent, QTcpSocket& socket);

private slots:

    void close();

    void readyRead();

private:
    void setError(QByteArray& buffer, RecordingController::Status status);

    RecordingController& parent_;
    QTcpSocket& socket_;
    QByteArray buffer_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
