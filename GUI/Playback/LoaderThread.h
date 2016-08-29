#ifndef SIDECAR_GUI_PLAYBACK_LOADERTHREAD_H // -*- C++ -*-
#define SIDECAR_GUI_PLAYBACK_LOADERTHREAD_H

#include "QtCore/QFileInfo"
#include "QtCore/QThread"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace Playback {

class Emitter;

class LoaderThread : public QThread
{
    Q_OBJECT
    using Super = QThread;
public:

    /** Log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    LoaderThread(Emitter* emitter, const QFileInfo& fileInfo);

    Emitter* getEmitter() const { return emitter_; }

private:

    void run();

    Emitter* emitter_;
    QFileInfo fileInfo_;
};

} // end namespace Playback
} // end namespace GUI
} // end namespace SideCar

#endif
