#ifndef SIDECAR_GUI_MASTER_CONFIGURATIONCONTROLLER_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_CONFIGURATIONCONTROLLER_H

#include "QtCore/QObject"
#include "QtCore/QModelIndex"
#include "QtCore/QString"
#include "QtCore/QStringList"

class QItemSelection;
class QTimer;

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace Master {

class Cleaner;
class ConfigurationInfo;
class ConfigurationModel;
class ConfigurationSettings;
class MainWindow;
class RunnerItem;

/** Controller class for the ConfigurationModel and the QTableView used to show loaded configuration files.
    Starts/stops configurations and keeps track of SideCar runner status for the running configurations.
*/
class ConfigurationController : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    /** Log device to use for RecordingController log messages.

        \return Log device
    */
    static Logger::Log& Log();

    /** Constructor.

        \param window 

        \param settings 
    */
    ConfigurationController(MainWindow& window,
                            ConfigurationSettings& settings);

    QStringList restore();

    QStringList getAllConfigurations() const;

    QStringList getViewableConfigurations() const;

    QStringList getViewableRunningConfigurations() const;

    QStringList getRecordableConfigurations() const;

    ConfigurationInfo* getConfigurationInfo(const QString& name) const;

    QStringList getInvalidRecordingDirectories() const;

    bool isAnyRecording() const;

signals:

    void activeConfigurationsChanged(const QStringList& active);

    void selectedConfigurationChanged(const ConfigurationInfo* info);

public slots:

    void foundRunner(const RunnerItem* added);

    void lostRunner(const RunnerItem* removed);

    void recordingStatusChanged(const QStringList& name, bool isRecording);

private slots:

    void add();

    void reload();

    void remove();

    void cleanup();

    void cleanupDone();

    void launchShutdown();

    void launchAll();

    void shutdownAll();

    void updateColumns(const QModelIndex& topLeft,
                       const QModelIndex& bottomRight);

    void currentSelectionChanged(const QModelIndex& current,
                                 const QModelIndex& previous);

    void editConfiguration(const QModelIndex& index);

    void adjustColumnSizes();

private:

    ConfigurationInfo* addPath(const QString& path, bool restoring);
    void updatePaths();
    void updateButtons();
    void updateStatus();
    void showMessage(const QString& string, int duration = 5000) const;
    bool eventFilter(QObject* object, QEvent* event);
    ConfigurationInfo* getCurrentConfiguration() const;

    MainWindow& mainWindow_;
    ConfigurationSettings& settings_;
    ConfigurationModel* model_;
    Cleaner* cleaner_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
