#include "RecordingInfo.h"

using namespace SideCar::GUI::Playback;

RecordingInfo::RecordingInfo(const QDir& recordingDir) :
    configurationNames_(), recordingDir_(recordingDir.absolutePath()), name_(recordingDir.dirName()), start_(),
    elapsed_(), dropCount_(0), dupeCount_(0)
{
    QStringList bits = recordingDir.dirName().split("-");
    if (bits.size() > 1) {
        QDateTime start = QDateTime::fromString(bits[0] + "-" + bits[1], "yyyyMMdd-HHmmss");
        start_ = QString("  %1  ").arg(start.time().toString());
    }
    loadFromFile();
}

void
RecordingInfo::loadFromFile()
{
    QDir dir(recordingDir_);
    QFile file(dir.filePath("notes.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QByteArray data;
    while (1) {
        data = file.readLine().trimmed();
        if (data.size() == 0) break;

        QList<QByteArray> bits = data.split(' ');
        if (bits.size() < 1) break;

        QString tag(bits[0]);
        if (tag == "Configurations:") {
            for (int index = 1; index < bits.size(); ++index) configurationNames_.append(bits[index]);
        } else if (tag == "Duration:") {
            if (bits.size() > 1) elapsed_ = QString("  %1  ").arg(QString(bits[1]));
        } else if (tag == "Dropped") {
            if (bits.size() > 2) dropCount_ = bits[2].toInt();
        } else if (tag == "Duplicate") {
            if (bits.size() > 2) dupeCount_ = bits[2].toInt();
        } else if (tag == "Radar") {
            tag = bits[1];
            if (tag == "Transmitting:") {
                if (bits.count() == 7) {
                    radarTransmitting_ = true;
                    radarFrequency_ = bits[5].toDouble();
                } else {
                    radarTransmitting_ = false;
                }
            } else if (tag == "Rotating:") {
                if (bits.count() == 7) {
                    radarRotating_ = true;
                    radarRotationRate_ = bits[5].toDouble();
                } else {
                    radarRotating_ = false;
                }
            }
        } else if (tag == "Using") {
            drfmOn_ = false;
            if (bits.count() > 3) {
                drfmOn_ = true;
                QString tmp("");
                for (int index = 5; index < bits.size(); ++index) {
                    tmp.append(bits[index]);
                    tmp.append(' ');
                }
                drfmConfig_ = tmp.trimmed();
            }
        }
    } // end while

    notes_ = file.readAll();
}

QString
RecordingInfo::getRadarFreqency() const
{
    return wasRadarTransmitting() ? QString::number(radarFrequency_) : QString("");
}

QString
RecordingInfo::getRadarRotationRate() const
{
    return wasRadarRotating() ? QString::number(radarRotationRate_) : QString("");
}
