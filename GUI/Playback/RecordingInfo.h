#ifndef SIDECAR_GUI_PLAYBACK_RECORDINGINFO_H // -*- C++ -*-
#define SIDECAR_GUI_PLAYBACK_RECORDINGINFO_H

#include "QtCore/QDateTime"
#include "QtCore/QDir"
#include "QtCore/QObject"
#include "QtCore/QStringList"
#include "QtCore/QTime"

namespace SideCar {
namespace GUI {
namespace Playback {

/** Class that holds information about a particular recording. Each recording started by the RecordingController
    gets its own RecordingInfo object, which is shown in the RecordingsView table.
*/
class RecordingInfo : public QObject
{
    Q_OBJECT
public:

    RecordingInfo(const QDir& recordingDir);

    const QString& getRecordingDirectory() const { return recordingDir_; }

    /** Obtain the configuration names that make up the recording.

        \return QStringList reference
    */
    const QStringList& getConfigurationNames() const
	{ return configurationNames_; }

    const QString& getName() const { return name_; }

    QString getStartTime() const { return start_; }

    const QString& getElapsedTime() const { return elapsed_; }

    const QString& getNotes() const { return notes_; }

    bool wasRadarTransmitting() const { return radarTransmitting_; }

    QString getRadarFreqency() const;

    bool wasRadarRotating() const { return radarRotating_; }

    QString getRadarRotationRate() const;

    bool wasDRFMOn() const { return drfmOn_; }

    QString getDRFMConfig() const { return drfmConfig_; }

    QString getBoolAsString(bool value) const;

    int getDropCount() const { return dropCount_; }

    int getDupeCount() const { return dupeCount_; }

signals:

    void statsChanged();

private:

    void loadFromFile();

    QStringList configurationNames_;
    QString recordingDir_;
    QString name_;
    int duration_;
    QString start_;
    QString elapsed_;
    int dropCount_;
    int dupeCount_;
    double radarFrequency_;
    double radarRotationRate_;
    QString drfmConfig_;
    QString notes_;
    bool radarTransmitting_;
    bool radarRotating_;
    bool drfmOn_;
};

} // end namespace Playback
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
