#include <fcntl.h> // for ::open

#include "boost/bind.hpp"

#include "QtCore/QFileInfo"
#include "QtCore/QString"
#include "QtGui/QStatusBar"

#include "GUI/LogUtils.h"
#include "GUI/Writers.h"
#include "IO/Decoder.h"
#include "IO/IndexMaker.h"
#include "IO/MessageManager.h"
#include "IO/TimeIndex.h"
#include "Messages/Header.h"
#include "Messages/MetaTypeInfo.h"
#include "Utils/FilePath.h"

#include "Clock.h"
#include "Emitter.h"
#include "MainWindow.h"

using namespace SideCar;
using namespace SideCar::GUI::Playback;

Logger::Log&
Emitter::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("playback.Emitter");
    return log_;
}

Emitter::Emitter(MainWindow* mainWindow, const QFileInfo& fileInfo, int row, bool emitting) :
    QThread(), clock_(mainWindow->getClock()), name_(fileInfo.baseName()), address_(mainWindow->getAddress()),
    suffix_(mainWindow->getSuffix()), metaTypeInfo_(0), reader_(), timeIndex_(0), writer_(0), pending_(0),
    loadPercentage_(0.0), subscriberCount_(0), row_(row), emitting_(emitting), valid_(false), running_(false)
{
    static Logger::ProcLog log("Emitter", Log());
    LOGINFO << "baseName: " << fileInfo.baseName() << std::endl;
}

Emitter::~Emitter()
{
    stop();
    if (writer_) {
        writer_->stop();
        delete writer_;
    }
}

void
Emitter::indexMakerUpdate(double percentageComplete)
{
    loadPercentage_ = percentageComplete;
    emit loadPercentageUpdate(row_);
}

void
Emitter::load(const QFileInfo& fileInfo)
{
    std::string path(fileInfo.absoluteFilePath().toStdString());
    Utils::FilePath fp(path);
    fp.setExtension(IO::TimeIndex::GetIndexFileSuffix());
    if (!fp.exists()) {
        IO::IndexMaker::Status status =
            IO::IndexMaker::Make(path, 15, boost::bind(&Emitter::indexMakerUpdate, this, _1));
        if (status != IO::IndexMaker::kOK) { return; }
    }

    timeIndex_ = new IO::TimeIndex(fp);
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd == -1) { return; }

    IO::FileReader reader;
    reader.getDevice().set_handle(fd);

    // Fetch the first record so we can get the file's metatype. Create a dummy Header object. We give it the Video
    // message meta-type to satisfy Header's API, but it is not relevant here; we could have given it any of the valid
    // meta-types.
    //
    Messages::Header header(*Messages::MetaTypeInfo::Find(Messages::MetaTypeInfo::Value::kVideo));
    if (!reader.fetchInput() || !reader.isMessageAvailable()) { return; }

    ACE_Message_Block* data = reader.getMessage();
    IO::Decoder decoder(data);
    header.load(decoder);

    // Now we have a valid Header. Fetch the MetaTypeInfo object it refers to.
    //
    metaTypeInfo_ = Messages::MetaTypeInfo::Find(header.getGloballyUniqueID().getMessageTypeKey());
    if (!metaTypeInfo_) { return; }

    // We are valid now. Assume the real emitting value.
    //
    ::lseek(fd, 0, SEEK_SET);

    // Connect signal handlers to the playback clock.
    //
    connect(clock_, SIGNAL(started()), SLOT(start()));
    connect(clock_, SIGNAL(stopped()), SLOT(stop()));
    connect(clock_, SIGNAL(playbackClockStartChanged(const Time::TimeStamp&)),
            SLOT(setPlaybackClockStart(const Time::TimeStamp&)));

    // Set our reader to use the given open file descriptor. The reader is set
    // to begin reading from the start of the file.
    //
    reader_.getDevice().set_handle(fd);

    // Read the first record to get its timestamp.
    //
    reader_.fetchInput();
    IO::MessageManager mgr(reader_.getMessage(), metaTypeInfo_);
    Messages::Header::Ref msg(mgr.getNative());
    startTime_ = msg->getCreatedTimeStamp();

    // Read the last record in the data file to get its timestamp.
    //
    ::lseek(fd, timeIndex_->getLastEntry(), SEEK_SET);
    if (reader_.fetchInput()) {
        IO::MessageManager mgr(reader_.getMessage(), metaTypeInfo_);
        Messages::Header::Ref msg(mgr.getNative());
        endTime_ = msg->getCreatedTimeStamp();
    }

    // Rewind to the file beginning.
    //
    ::lseek(fd, 0, SEEK_SET);

    // Create a message writer that will send out the messages at the appropriate time.
    //
    valid_ = true;
    indexMakerUpdate(0.0);
}

void
Emitter::makeWriter(bool restartIfRunning)
{
    static Logger::ProcLog log("makeWriter", Log());
    LOGINFO << name_ << " restartIfRunning: " << restartIfRunning << std::endl;

    if (!valid_ || !emitting_) { return; }

    // Make sure we are not running when we create a new UDPMessageWriter. Remember if we were running in case we are
    // permitted to restart.
    //
    bool wasRunning = running_;
    if (wasRunning) stop();

    if (writer_) {
        writer_->stop();
        delete writer_;
        writer_ = 0;
    }

    // Create our published name.
    //
    QString name(name_);
    if (!suffix_.isEmpty()) { name += suffix_; }

    subscriberCount_ = 0;
    writer_ = UDPMessageWriter::Make(name, metaTypeInfo_->getName(), address_);
    if (!writer_) {
        LOGERROR << "failed to create writer!" << std::endl;
        return;
    }

    connect(writer_, SIGNAL(subscriberCountChanged(size_t)), SLOT(writerSubscriberCountChanged(size_t)));

    // Restart our own emitter thread if we were running and we can restart.
    //
    if (wasRunning && restartIfRunning) { start(); }
}

void
Emitter::writerSubscriberCountChanged(size_t value)
{
    subscriberCount_ = value;
    emit subscriberCountChanged(row_);
}

void
Emitter::setEmitting(bool emitting)
{
    static Logger::ProcLog log("setEmitting", Log());
    LOGINFO << "emitting: " << emitting << std::endl;

    if (emitting == emitting_) return;
    emitting_ = emitting;
    if (!valid_) return;

    if (emitting) {
        if (!writer_) { makeWriter(); }

        if (clock_->isRunning()) {
            setPlaybackClockStart(clock_->getPlaybackClock());
            start();
        }
    } else {
        // Stop emitting and remove our writer.
        //
        stop();

        // Remove any current writer.
        //
        if (writer_) {
            writer_->stop();
            delete writer_;
            writer_ = 0;
            writerSubscriberCountChanged(0);
        }
    }
}

void
Emitter::setAddress(const QString& address)
{
    if (address != address_) {
        address_ = address;
        makeWriter(true);
    }
}

void
Emitter::setSuffix(const QString& suffix)
{
    if (suffix != suffix_) {
        suffix_ = suffix;
        makeWriter(true);
    }
}

void
Emitter::setPlaybackClockStart(const Time::TimeStamp& playbackClockStart)
{
    static Logger::ProcLog log("setPlaybackClockStart", Log());
    LOGINFO << "playbackClockStart: " << playbackClockStart << std::endl;

    if (!valid_) { return; }

    if (running_) { stop(); }

    if (!writer_) { makeWriter(); }

    // Obtain the position of a record that is close to the given time (but not
    // greater than it) and seek to it.
    //
    off_t pos = timeIndex_->findOnOrBefore(playbackClockStart.getSeconds());
    repositionAndFetch(pos, playbackClockStart);
}

void
Emitter::start()
{
    static Logger::ProcLog log("start", Log());
    LOGINFO << name_ << ' ' << running_ << std::endl;

    if (!valid_ || !emitting_) {
        LOGERROR << name_ << " not valid or not emitting" << std::endl;
        return;
    }

    setPlaybackClockStart(clock_->getPlaybackClock());

    // We only run if we have data to emit.
    //
    if (pending_) {
        LOGDEBUG << name_ << " has pending - will start" << std::endl;
        running_ = true;
        Super::start();
        LOGDEBUG << name_ << " started" << std::endl;
    } else {
        LOGDEBUG << name_ << " not started - emitting: " << emitting_ << " pending: " << pending_ << std::endl;
    }
}

void
Emitter::stop()
{
    static Logger::ProcLog log("stop", Log());

    if (running_) {
        running_ = false;
        wait();
        LOGDEBUG << name_ << " stopped" << std::endl;
    } else {
        LOGDEBUG << name_ << " not running" << std::endl;
    }
}

void
Emitter::repositionAndFetch(off_t pos, const Time::TimeStamp& when)
{
    static Logger::ProcLog log("repositionAndFetch", Log());
    LOGINFO << "pos: " << pos << " when: " << when << std::endl;

    if (!valid_) return;

    if (pending_) {
        pending_->release();
        pending_ = 0;
    }

    int fd = reader_.getDevice().get_handle();
    if (fd == -1) {
        LOGERROR << "invalid fd" << std::endl;
        return;
    }

    // Move to given position in the file.
    //
    ::lseek(fd, pos, SEEK_SET);

    // Now continue reading until we've reached the record that is beyond the time we are seeking. If our indexing is
    // not too coarse, this should be fairly quick.
    //
    Messages::Header header(*metaTypeInfo_);
    ACE_Message_Block* last = 0;
    while (reader_.fetchInput() && reader_.isMessageAvailable()) {
        ACE_Message_Block* data = reader_.getMessage();
        IO::Decoder decoder(data->duplicate());
        header.load(decoder);
        if (header.getCreatedTimeStamp() > when) {
            if (!last) last = data;
            break;
        }

        if (last) last->release();
        last = data;
    }

    if (last) {
        // Found something to emit. Record the message found and its timestamp.
        //
        IO::MessageManager mgr(last, metaTypeInfo_);
        Messages::Header::Ref msg(mgr.getNative());
        LOGDEBUG << "sequence: " << msg->getGloballyUniqueID().getMessageSequenceNumber()
                 << " time: " << msg->getCreatedTimeStamp() << std::endl;
        pending_ = mgr.getMessage();
        pendingTime_ = header.getCreatedTimeStamp();
    }
}

void
Emitter::run()
{
    // Maximum amount of time to sleep while waiting to emit a message.
    //
    static const Time::TimeStamp kMaxSleep(0, Time::TimeStamp::kMicrosPerSecond / 10); // 0.1 seconds

    static Logger::ProcLog log("run", Log());
    LOGINFO << name_ << " valid: " << valid_ << " emitting: " << emitting_ << " writer: " << writer_
            << " pending: " << pending_ << std::endl;

    // !!! This should never be true here
    //
    if (!valid_ || !emitting_) {
        LOGERROR << "exiting" << std::endl;
        return;
    }

    // Keep running while our running_ attribute is true and we have a message to emit.
    //
    while (running_ && pending_) {
        // Calculate amount of wall time until the next emission.
        //
        Time::TimeStamp duration(clock_->getWallClockDurationUntil(pendingTime_));
        LOGDEBUG << name_ << " clock: " << clock_->getPlaybackClock() << " pendingTime: " << pendingTime_
                 << " duration: " << duration << std::endl;

        // Do we need to sleep?
        //
        if (duration > Time::TimeStamp::Min()) {
            // Don't sleep too long in order to detect running_ changes. Since we know there will be more time to wait,
            // we 'continue' back to beginning of the while loop once we wake up.
            //
            if (duration > kMaxSleep) {
                usleep(kMaxSleep.getMicro());
                continue;
            }

            // Sleep for the required number of microseconds. When we awake, we will emit the pending message.
            //
            usleep(duration.getSeconds() * Time::TimeStamp::kMicrosPerSecond + duration.getMicro());
        }

        // Time to emit the message.
        //
        LOGDEBUG << name_ << " emitting " << pendingTime_ << std::endl;
        IO::MessageManager mgr(pending_);
        if (subscriberCount_) { writer_->writeMessage(mgr); }
        pending_ = 0;

        // Fetch the next message to emit.
        //
        if (reader_.fetchInput()) {
            IO::MessageManager mgr(reader_.getMessage(), metaTypeInfo_);
            pending_ = mgr.getMessage();
            pendingTime_ = mgr.getNative()->getCreatedTimeStamp();
            LOGDEBUG << name_ << " pendingTime: " << pendingTime_ << std::endl;
        }
    }

    // !!! Don't clear 'running_' flag when we exit. That way, the main thread will reap us with a wait() call.
    //
    LOGDEBUG << name_ << " thread exiting" << std::endl;
}

QString
Emitter::getFormattedStartTime() const
{
    if (!valid_) return "NA";
    return clock_->formatTimeStamp(startTime_);
}

QString
Emitter::getFormattedEndTime() const
{
    if (!valid_) return "NA";
    return clock_->formatTimeStamp(endTime_);
}

QString
Emitter::getFormattedDuration() const
{
    if (!valid_) return "NA";
    Time::TimeStamp duration(endTime_);
    duration -= startTime_;
    return clock_->formatDuration(duration);
}

int
Emitter::getPort() const
{
    return writer_ ? writer_->getPort() : 0;
}
