#ifndef SIDECAR_GUI_MASTER_RECORDINGCONTROLLER_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_RECORDINGCONTROLLER_H

#include "QtCore/QFileInfo"
#include "QtCore/QModelIndex"
#include "QtCore/QObject"
#include "QtCore/QString"

class QMessageBox;
class QTcpServer;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Master {

class MainWindow;
class ConfigurationController;
class RecordingInfo;
class RecordingModel;

class RecordingController : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    enum Status {
        kOK = 0,
        kNoLoadedConfig,
        kFailedCreateDirectory,
        kFailedRecordingSetup,
        kFailedPostRecordingStateChange
    };

    enum { kServerPort = 4465 };

    /** Log device to use for RecordingController log messages.

        \return Log device
    */
    static Logger::Log& Log();

    RecordingController(MainWindow& mainWindow);

    QStringList restore(const ConfigurationController& controller);

    QStringList restore(const QString& path);

    QString getNow() const;

    Status start(const QStringList& configNames);

    Status start();

    Status stop(bool byDuration = false);

    void restoreActiveRecording();

    RecordingInfo* getActiveRecording() const { return activeRecording_; }

    RecordingInfo* getLastRecording() const { return lastRecording_; }

    RecordingInfo* getRecordingInfo(const QString& name) const;

    void reset();

signals:

    void statusChanged(const QStringList& configNames, bool isRecording);

public slots:

    void startStop();

private slots:

    void remoteConnection();

    void editNotes(const QModelIndex& index);

    void doNowTick(const QString& now);

    void updateColumns(const QModelIndex& topLeft, const QModelIndex& bottomRight);

    void promptDone(int resultCode);

private:
    void showMessage(const QString& string, int duration = 5000) const;

    bool eventFilter(QObject* object, QEvent* event);

    bool showError(const QString& title, const QString& text);

    void deleteSelectedRecording();

    void adjustColumnSizes();

    MainWindow& mainWindow_;
    RecordingModel* model_;
    QTcpServer* server_;
    RecordingInfo* activeRecording_;
    RecordingInfo* lastRecording_;
    QMessageBox* prompt_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
