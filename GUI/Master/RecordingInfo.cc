#include "QtGui/QMessageBox"

#include "MainWindow.h"
#include "NotesWindow.h"
#include "RecordingController.h"
#include "RecordingInfo.h"

using namespace SideCar::GUI::Master;

// static QChar kBullet = QChar(0x2022);
// static QChar kBullet = QChar(0x006C);

static QChar kBullet = QChar(0x2713);
static QChar kSpace = QChar(0x0020);

RecordingInfo::RecordingInfo(RecordingController& controller, const QString& name, const QTime& duration) :
    configurationNames_(), recordingDirs_(), name_(name), duration_(0), start_(QDateTime::currentDateTime().toUTC()),
    elapsed_("00:00:00"), remaining_(""), notesWindow_(0), dropCount_(0), dupeCount_(0),
    hasDuration_(duration.isValid()), done_(false)
{
    if (hasDuration_) {
        duration_ = duration.hour() * 3600 + duration.minute() * 60 + duration.second();
        remaining_ = duration.toString(" -hh:mm:ss");
    }

    notesWindow_ = new NotesWindow(controller, *this);
    notesWindow_->initialize();
}

RecordingInfo::RecordingInfo(RecordingController& controller, const QDir& recordingDir) :
    configurationNames_(), recordingDirs_(), name_(recordingDir.dirName()), duration_(0), start_(), elapsed_(),
    notesWindow_(0), dropCount_(0), dupeCount_(0), hasDuration_(false), done_(true)
{
    QStringList bits = recordingDir.dirName().split("-");
    if (bits.size() > 1) start_ = QDateTime::fromString(bits[0] + "-" + bits[1], "yyyyMMdd-HHmmss");
    recordingDirs_.append(recordingDir.absolutePath());
    notesWindow_ = new NotesWindow(controller, *this);
    notesWindow_->initialize();
    loadFromFile();
}

RecordingInfo::~RecordingInfo()
{
    delete notesWindow_;
    notesWindow_ = 0;
}

void
RecordingInfo::loadFromFile()
{
    for (int index = 0; index < recordingDirs_.size(); ++index) {
        QDir dir(recordingDirs_[index]);
        QFile file(dir.filePath("notes.txt"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data;
            QList<QByteArray> bits;

            // Read in the configurations
            //
            data = file.readLine().trimmed();
            bits = data.split(' ');
            for (int index = 1; index < bits.size(); ++index) configurationNames_.append(bits[index]);

            // Read in the elapsed time
            //
            data = file.readLine().trimmed();
            bits = data.split(' ');
            if (bits.size() > 1) elapsed_ = bits[1];

            // Read in the drop count
            //
            data = file.readLine().trimmed();
            bits = data.split(' ');
            if (bits.size() > 2) dropCount_ = bits[2].toInt();

            // Read in the dupe count
            //
            data = file.readLine().trimmed();
            bits = data.split(' ');
            if (bits.size() > 2) dupeCount_ = bits[2].toInt();

            // Parse the rest inside the notes window.
            //
            notesWindow_->loadFromFile(file);
            return;
        }
    }
}

void
RecordingInfo::addConfiguration(const QString& configName)
{
    configurationNames_.append(configName);
}

void
RecordingInfo::addRecordingDirectory(const QDir& dir)
{
    QString path(dir.absolutePath());
    if (!recordingDirs_.contains(path)) recordingDirs_.append(path);
}

void
RecordingInfo::infoChanged()
{
    emit configChanged();
}

void
RecordingInfo::showNotesWindow()
{
    notesWindow_->showAndRaise();
}

void
RecordingInfo::closeNotesWindow()
{
    notesWindow_->close();
}

void
RecordingInfo::recordingStopped()
{
    done_ = true;
    notesWindow_->recordingStopped();
    emit configChanged();
}

bool
RecordingInfo::hasDurationPassed(const QString& now)
{
    if (done_) return false;

    int elapsed = start_.secsTo(QDateTime::currentDateTime());
    if (hasDuration_) {
        int remaining = duration_ - elapsed;
        if (remaining <= 0) {
            done_ = true;
            remaining_ = "";
        } else {
            remaining_ = QTime(0, 0).addSecs(remaining).toString(" -hh:mm:ss");
        }
    }

    elapsed_ = QTime(0, 0).addSecs(elapsed).toString("hh:mm:ss");
    notesWindow_->recordingTick(now);
    return done_;
}

bool
RecordingInfo::wasRadarTransmitting() const
{
    return notesWindow_->wasRadarTransmitting();
}

QString
RecordingInfo::getBoolAsString(bool value) const
{
    return value ? kBullet : kSpace;
}

QString
RecordingInfo::getRadarFreqency() const
{
    return wasRadarTransmitting() ? QString::number(notesWindow_->getRadarFrequency()) : QString("");
}

bool
RecordingInfo::wasRadarRotating() const
{
    return notesWindow_->wasRadarRotating();
}

QString
RecordingInfo::getRadarRotationRate() const
{
    return wasRadarRotating() ? QString::number(notesWindow_->getRadarRotationRate()) : QString("");
}

bool
RecordingInfo::wasDRFMOn() const
{
    return notesWindow_->wasDRFMOn();
}

QString
RecordingInfo::getDRFMConfig() const
{
    return notesWindow_->getDRFMConfig();
}

void
RecordingInfo::updateStats(int dropCount, int dupeCount)

{
    bool changed = false;
    if (dropCount_ == -1 || dropCount_ != dropCount) {
        dropCount_ = dropCount;
        changed = true;
    }

    if (dupeCount_ == -1 || dupeCount_ != dupeCount) {
        dupeCount_ = dupeCount;
        changed = true;
    }

    if (changed) emit statsChanged();
}

void
RecordingInfo::start(const QStringList& changes)
{
    notesWindow_->start(changes);
}

void
RecordingInfo::addChangedParameters(const QStringList& changes)
{
    notesWindow_->addChangedParameters(changes);
}

void
RecordingInfo::addAlerts(const QStringList& alerts)
{
    notesWindow_->addAlerts(alerts);
}
