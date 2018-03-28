#ifndef SIDECAR_GUI_MASTER_NOTESWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_NOTESWINDOW_H

class QDir;
class QFile;

#include "GUI/MainWindowBase.h"

namespace Ui {
class NotesWindow;
}
namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Master {

class RecordingController;
class RecordingInfo;

class NotesWindow : public MainWindowBase {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    static Logger::Log& Log();

    /** Constructor.
     */
    NotesWindow(RecordingController& controller, RecordingInfo& info);

    void recordingTick(const QString& now);

    void recordingStopped();

    bool wasRadarTransmitting() const;

    double getRadarFrequency() const;

    bool wasRadarRotating() const;

    double getRadarRotationRate() const;

    bool wasDRFMOn() const;

    QString getDRFMConfig() const;

    void loadFromFile(QFile& file);

public slots:

    void start(const QStringList& changes);

    void addTimeStampedEntry(const QString& value);

    void addChangedParameters(const QStringList& changes);

    void addAlerts(const QStringList& alerts);

    void addAlert(const QString& alert);

private slots:

    void on_insertNow__clicked();

    void on_insertElapsed__clicked();

    void radarTransmittingChanged();

    void radarRotatingChanged();

    void drfmOnChanged();

    void statsChanged();

private:
    void makeMenuBar();

    void showRecordingStatus(bool recording);

    void closeEvent(QCloseEvent* event);

    void saveToFile();

    Ui::NotesWindow* gui_;
    RecordingController& controller_;
    RecordingInfo& info_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
