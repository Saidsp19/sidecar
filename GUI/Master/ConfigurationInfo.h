#ifndef SIDECAR_GUI_MASTER_CONFIGURATIONINFO_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_CONFIGURATIONINFO_H

#include "QtCore/QDateTime"
#include "QtCore/QObject"
#include "QtCore/QTimer"
#include "QtCore/QString"
#include "QtCore/QStringList"

#include "Configuration/Loader.h"

namespace Logger { class Log; }
namespace SideCar {
namespace GUI {

namespace Master {

class ConfigurationEditor;
class ConfigurationModel;
class DiskSpaceThread;
class MainWindow;
class RunnerItem;

/** Class that holds information about a particular configuration file. Also manages startup / shutdown for
    remote Runner processes.
*/
class ConfigurationInfo : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    /** Set of values for the ocnfiguration status.
     */
    enum Status {
	kError,			///< Configuration has an error
	kNotRunning,		///< Configuration is not running
	kPartial,		///< Configuration is running but incomplete
	kRunning,		///< Configuration is running and complete
	kRecording
    };

    /** Log device for ConfigurationInfo instances

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.

        \param path location of the XML configuration file to load
    */
    ConfigurationInfo(QObject* parent, ConfigurationModel* model,
                      const QString& path);

    /** Destructor.
     */
    ~ConfigurationInfo();

    /** Obtain an editor for the configuration file.

        \return ConfigurationEditor instance.
    */
    ConfigurationEditor* getEditor();

    /** Obtain the current configuration status.

        \return current status
    */
    Status getStatus() const
	{ return isRecording_ ? kRecording : status_; }

    /** Obtain the error text generated when the configuration file was loaded.

        \return error text
    */
    QString getErrorText() const { return errorText_; }

    /** Get the status text for the configuration, based on the current status value. This value appears in the
        configuration status table in the main window.

        \return status text
    */
    QString getStatusText() const;

    /** Obtain the list of host names that run remote Runner processes as defined in the loaded configuration
        file.

        \return host name list
    */
    QStringList getHostNames() const { return loader_.getHostNames(); }
    
    /** Obtain the list of files that the configuration file included during loading.

        \return include path list
    */
    QStringList getIncludePaths() const { return loader_.getIncludePaths(); }

    /** Obtain the name of the configuration file. This value appears in the confiugration status table in the
        main window.

        \return configuration name
    */
    QString getName() const { return loader_.getConfigurationName(); }

    /** Obtain the path of the configuraton file.

        \return configuraton path
    */
    QString getPath() const { return loader_.getConfigurationPath(); }

    /** Obtain the log directory defined in the configuration file.

        \return directory path
    */
    QString getLogsDirectory() const
	{ return loader_.getLogsDirectory(); }

    /** Obtain the recording directory defined in the configuration file.

        \return directory path
    */
    QString getRecordingsDirectory() const
	{ return loader_.getRecordingsDirectory(); }

    /** Obtain the perentage of recording space in use.

        \return value between 0 and 100
    */
    double getRecordingDirPercentUsed() const
	{ return recordingDirPercentUsed_; }

    /** Obtain the amount of free space in the recording path. Formatted as a numeric value followed by a unit,
        such as 'MB' or 'GB'.

        \return formatted disk space
    */
    QString getRecordingDirFreeSpace() const
	{ return recordingDirFreeSpace_; }

    /** Determine if the recording directory exists on the system.

        \return true if so
    */
    bool hasValidRecordingDirectory() const;

    /** Attempt to reload the configuration file.

        \return true if successsful
    */
    bool reload() { return load(false); }

    /** Load the configuration file.

        \param hideErrors don't show syntax error prompt if true

        \return true if successsful
    */
    bool load(bool hideErrors);

    /** Start the remote Runner processes defined by the configuration file.

        \return true if successsful
    */
    bool startup();

    /** Change the status to the given value

        \param status new value to use
    */
    void setStatus(Status status);

    /** Set a flag indicating that the configuration is shutting down. Inhibits the error prompt that shows up
	when a remote process exits.
    */
    void shuttingDown() { shuttingDown_ = true; }

    /** Determine if the configuration file is currently starting up.

        \return true if so
    */
    bool isStartingUp() const { return startupTimer_->isActive(); }

    /** Determine if the configuration file is currently shutting down.

        \return true if so
    */
    bool isShuttingDown() const { return shuttingDown_; }

    /** Determine if there was an error loading the configuration file.

        \return true if so
    */
    bool hasError() const { return status_ == kError; }

    /** Determine if the configuration is not running at all.

        \return true if so
    */
    bool isNotRunning() const { return status_ <= kNotRunning; }

    /** Determine if the configuration has at least one Runner running.

        \return true if so
    */
    bool isRunning() const { return status_ >= kPartial; }

    /** Determine if the configuration is currently recording.

        \return true if so
    */
    bool isRecording() const { return isRecording_; }

    /** Change the recording state of the configuration.

        \param state new state
    */
    void setRecordingState(bool state);

    /** The Master application detected status for a remote Runner. Update the status of this configuration.

        \param runner name of the remote Runner that was found
    */
    void foundRunner(const RunnerItem* runner);

    /** The Master application detected that a remote Runner is no longer running. Update the status of this
        configuration.

	\param runner name of the remote Runner that was found
    */
    void lostRunner(const QString& runner);

    /** Determine if the configuration is recordable

        \return true if so
    */
    bool isRecordable() const { return recordable_; }

    /** Change the recordability for this configuration.

        \param state new state
    */
    void setRecordable(bool state);

    /** Determine if the Runner status should be viewable for this configuration.

        \return true if so
    */
    bool isViewable() const { return viewable_; }

    /** Change the visibility of the Runner status for this configuration.

        \param state new state.
    */
    void setViewable(bool state);

    /** Determin if this configuration can record data.

        \return 
    */
    bool canRecord() const
	{ return isRecordable() && hasValidRecordingDirectory(); }

    /** Obtain the current number of found remote Runner processes.

        \return count
    */
    int getRunningRunnerCount() const { return runningCount_; }

    /** Obtain the configured number of remote Runner processes.

        \return count
    */
    int getConfiguredRunnerCount() const { return running_.size(); }

public slots:

    /** Slot for disk space updates.

        \param percentUsed current percentage used

        \param freeSpace current free space available
    */
    void diskSpaceUpdate(double percentUsed, QString freeSpace);

private slots:

    /** 

        \param wasCanceled 
    */
    void launcherDone(bool wasCanceled);

    void startupTimerDone();

    void partialNotify();

private:

    void checkRunningCount();

    bool setLoadError(bool noPrompt, const QString& text);

    ConfigurationModel* model_;
    ConfigurationEditor* editor_;
    DiskSpaceThread* diskSpaceThread_;
    QString configurationPath_;
    Configuration::Loader loader_;
    Status status_;
    double recordingDirPercentUsed_;
    QString recordingDirFreeSpace_;
    QString errorText_;
    QList<bool> running_;
    int runningCount_;
    QTimer* startupTimer_;
    QDateTime lastLoaded_;
    bool isRecording_;
    bool shuttingDown_;
    bool recordable_;
    bool viewable_;
    bool started_;
    
    static bool ShowError(const QString& title, const QString& text);
    static MainWindow* GetMainWindow();

    static QString kStatusText_[];
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
