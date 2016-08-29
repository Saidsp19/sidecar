#ifndef SIDECAR_GUI_HEALTHANDSTATUS_CHANNELCONNECTION_H // -*- C++ -*-
#define SIDECAR_GUI_HEALTHANDSTATUS_CHANNELCONNECTION_H

#include "QtCore/QObject"
#include "QtCore/QSize"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class MessageList;
class ServiceEntry;
class Subscriber;

namespace HealthAndStatus {

class ChannelPlotSettings;

class ChannelConnection : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    static Logger::Log& Log();

    ChannelConnection(const QString& name, QObject* parent = 0);

    ~ChannelConnection();

    const QString& getName() const { return name_; }

    void useServiceEntry(ServiceEntry* serviceEntry);

    bool isConnected() const;

    void setMinimumSizeHint(const QSize& size) { minimumSizeHint_ = size; }

    const QSize& getMinimumSizeHint() const { return minimumSizeHint_; }

    ChannelPlotSettings* getSettings() const { return settings_; }

signals:

    void connected();

    void incoming(const MessageList& msgs);

    void disconnected();

public slots:

    void shutdown();

private slots:

    void readerIncoming();

private:
    ChannelPlotSettings* settings_;
    Subscriber* reader_;
    QString name_;
    QSize minimumSizeHint_;
};

} // end namespace HealthAndStatus
} // end namespace GUI
} // end namespace SideCar

#endif
