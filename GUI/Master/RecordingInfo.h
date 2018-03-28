#ifndef SIDECAR_GUI_MASTER_RECORDINGINFO_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_RECORDINGINFO_H

#include "QtCore/QDateTime"
#include "QtCore/QDir"
#include "QtCore/QObject"
#include "QtCore/QStringList"
#include "QtCore/QTime"

namespace SideCar {
namespace GUI {
namespace Master {

class NotesWindow;
class RecordingController;

/** Class that holds information about a particular recording. Each recording started by the RecordingController
    gets its own RecordingInfo object, which is shown in the RecordingsView table.
*/
class RecordingInfo : public QObject {
    Q_OBJECT
public:
    /** Constructor

        \param parent container hosting the widget

        \param filter

        \param duration

        \param dir
    */
    RecordingInfo(RecordingController& controller, const QString& name, const QTime& duration);

    RecordingInfo(RecordingController& controller, const QDir& recordingDir);

    /** Destructor. Deletes the NotesWindow.
     */
    ~RecordingInfo();

    void addConfiguration(const QString& configName);

    void addRecordingDirectory(const QDir& recordingDir);

    /** Obtain the configuration names that make up the recording.

        \return QStringList reference
    */
    const QStringList& getConfigurationNames() const { return configurationNames_; }

    const QStringList& getRecordingDirectories() const { return recordingDirs_; }

    const QString& getName() const { return name_; }

    QString getStartTime() const { return start_.time().toString(); }

    QString getFormattedStartTime() const { return start_.time().toString() + " UTC"; }

    const QString& getElapsedTime() const { return elapsed_; }

    QString getFormattedElapsedTime() const { return QString("+") + elapsed_; }

    const QString& getRemainingTime() const { return remaining_; }

    /** Determine if this recording is finished.

        \return true if so
    */
    bool isDone() const { return done_; }

    /** Show the NotesWindow window associated with this recording.
     */
    void showNotesWindow();

    void closeNotesWindow();

    void start(const QStringList& changes);

    void addChangedParameters(const QStringList& changes);

    void addAlerts(const QStringList& alerts);

    /** Determine if the set duration has passed for this recording.

        \return
    */
    bool hasDurationPassed(const QString& now);

    void recordingStopped();

    bool wasRadarTransmitting() const;

    QString getRadarFreqency() const;

    bool wasRadarRotating() const;

    QString getRadarRotationRate() const;

    bool wasDRFMOn() const;

    QString getDRFMConfig() const;

    QString getBoolAsString(bool value) const;

    int getDropCount() const { return dropCount_; }

    int getDupeCount() const { return dupeCount_; }

    void updateStats(int dropCount, int dupeCount);

signals:

    void configChanged();

    void statsChanged();

public slots:

    void infoChanged();

private:
    void loadFromFile();

    QStringList configurationNames_;
    QStringList recordingDirs_;
    QString name_;
    int duration_;
    QDateTime start_;
    QString elapsed_;
    QString remaining_;
    NotesWindow* notesWindow_;
    int dropCount_;
    int dupeCount_;
    bool hasDuration_;
    bool done_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
