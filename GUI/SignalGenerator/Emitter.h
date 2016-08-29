#ifndef SIDECAR_GUI_SIGNALGENERATOR_EMITTER_H // -*- C++ -*-
#define SIDECAR_GUI_SIGNALGENERATOR_EMITTER_H

#include "QtCore/QList"
#include "QtCore/QMutex"
#include "QtCore/QThread"

#include "Messages/Video.h"

#include "MainWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class MessageWriter;

namespace SignalGenerator {

class Emitter : public QThread
{
    Q_OBJECT
    using Super = QThread;
public:

    static Logger::Log& Log();

    Emitter();

    ~Emitter();

    void setFrequency(size_t frequency);

    void clear();

    void addMessage(const Messages::Video::Ref& msg);

    void setMessageList(const QList<Messages::Video::Ref>& messages);

    MessageWriter* setPublisherInfo(const QString& name,
                                    MainWindow::ConnectionType connectionType,
                                    const QString& multicastAddress);

    bool start();

    bool stop();

    void rewind();

    bool isRunning() const { return running_; }

    int getMessageCount() const { return messages_.size(); }

private:

    void run();

    MessageWriter* writer_;
    useconds_t sleepDuration_;
    QList<Messages::Video::Ref> messages_;
    int messageIndex_;
    volatile bool running_;
    QMutex mutex_;
};

} // end namespace SignalGenerator
} // end namespace GUI
} // end namespace SideCar

#endif
