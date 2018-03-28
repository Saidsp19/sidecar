#ifndef SIDECAR_GUI_MASTER_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_MAINWINDOW_H

#include "QtCore/QList"
#include "QtCore/QPointer"
#include "QtCore/QStringList"

#include "GUI/MainWindowBase.h"
#include "GUI/ServiceBrowser.h"

#include "App.h"

#include "ui_MainWindow.h"

class QDir;
class QProgressDialog;

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {
namespace Master {

class Configuration;
class ConfigurationController;
class ConfigurationInfo;
class RecordingController;
class ServicesModel;
class StatusWidget;
class UtilsWidget;

class MainWindow : public MainWindowBase, public Ui::MainWindow {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    static Logger::Log& Log();

    /** Constructor.
     */
    MainWindow();

    /** Obtain the App object singleton.

        \return App object
    */
    App* getApp() const { return App::GetApp(); }

    QString getNow() const { return now_; }

    void setRecordingElapsed(const QString& elapsed, const QString& remaining);

    ConfigurationController& getConfigurationController() const { return *configurationController_; }

    RecordingController& getRecordingController() const { return *recordingController_; }

    QStringList getRecordableConfigurations() const;

    ConfigurationInfo* getConfigurationInfo(const QString& name) const;

    bool shutdownConfiguration(const QString& configName);

    bool getChangedParameters(const QStringList& configNames, QStringList& changes) const;

    bool startRecording(const QStringList& configNames, const QStringList& recordingPaths);

    bool stopRecording();

    bool stopRecording(const QStringList& filter);

    bool isCalibrating(const QString& configName) const;

    ServicesModel* getServicesModel() const { return servicesModel_; }

    UtilsWidget* getUtilsWidget() const { return utilsWidget_; }

    int getNumActiveMasters() const { return activeMastersCount_; }

    bool isAnyActive() const;

signals:

    void nowTick(const QString& now);

public slots:

    void showMessage(const QString& text, int duration = 5000);

    void forceShutdown(ConfigurationInfo* config);

    void updateRecordingStartStop();

    void servicesStatusUpdated();

    void parameterValuesChanged(const QStringList& changes);

    void filterRunners();

    void clearStats();

private slots:

    void updateActiveMasters(const ServiceEntryHash& available);

    void configurationStatusChanged(const QStringList& active);

    void changeRunState(int index);

    void restoreConfigurations();

    void restoreRecordings();

private:
    void processChecker();

    void restoreFromSettings(QSettings& settings);

    void saveToSettings(QSettings& settings);

    void doNowTick();

    void showRecordingState(bool recording);

    void timerEvent(QTimerEvent* event);

    void showEvent(QShowEvent* event);

    void closeEvent(QCloseEvent* event);

    bool eventFilter(QObject* obj, QEvent* event);

    StatusWidget* statusWidget_;
    UtilsWidget* utilsWidget_;
    ServicesModel* servicesModel_;
    ServiceBrowser* browser_;
    ConfigurationController* configurationController_;
    RecordingController* recordingController_;
    QProgressDialog* checker_;
    QString zoneName_;
    QString now_;
    int nowTimerId_;
    int lastFailureCount_;
    int activeMastersCount_;

    int preDrops_;
    int preDupes_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
