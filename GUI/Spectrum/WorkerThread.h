#ifndef SIDECAR_GUI_SPECTRUM_WORKERTHREAD_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_WORKERTHREAD_H

#include "QtCore/QThread"

#include "Messages/Video.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Spectrum {

class WorkRequest;

class WorkerThread : public QThread {
    Q_OBJECT
    using Super = QThread;

public:
    /** Log device to use for WorkerThread log messages.

        \return Log device
    */
    static Logger::Log& Log();

    WorkerThread(WorkRequest* workRequest);

    void stop();

    void submit();

    WorkRequest* getWorkRequest() { return workRequest_; }

signals:

    void threadSubmit();

    void finished();

private slots:

    void threadProcess();

private:
    void run();

    WorkRequest* workRequest_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
