#include "QtCore/QSettings"

#include "GUI/LogUtils.h"

#include "App.h"
#include "AzimuthLatch.h"
#include "ChannelConnection.h"
#include "History.h"
#include "HistoryPosition.h"
#include "Visualizer.h"

using namespace SideCar;
using namespace SideCar::GUI::AScope;

static const char* const kAzimuthLatchEnabled = "AzimuthLatchEnabled";
static const char* const kAzimuthLatchAzimuth = "AzimuthLatchAzimuth";
static const char* const kAzimuthLatchRelatch = "AzimuthLatchRelatch";
static const char* const kAzimuthLatchChannel = "AzimuthLatchChannel";

Logger::Log&
HistoryPosition::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.HistoryPosition");
    return log_;
}

HistoryPosition::HistoryPosition(Visualizer* parent, AzimuthLatch* azimuthLatch) :
    QObject(parent), parent_(parent), azimuthLatch_(azimuthLatch), history_(App::GetApp()->getHistory()), position_(0),
    viewingPast_(false), synchronized_(true), infoReporter_(false)
{
    static Logger::ProcLog log("HistoryPosition", Log());
    LOGINFO << std::endl;
    connect(parent_, SIGNAL(channelConnectionsChanged(const QSet<QString>&)),
            SLOT(channelNamesChanged(const QSet<QString>&)));
    connect(&history_, SIGNAL(liveFrameChanged()), SLOT(liveFrameChanged()));
    connect(&history_, SIGNAL(pastFrozen()), SLOT(pastFrozen()));
    connect(&history_, SIGNAL(pastThawed()), SLOT(pastThawed()));

    connect(azimuthLatch_, SIGNAL(configurationChanged(bool, double, bool, const QString&)),
            SLOT(azimuthLatchChanged(bool, double, bool, const QString&)));

    azLatchEnabled_ = false;
    azLatchAzimuth_ = azimuthLatch->getAzimuth();
    azLatchRelatch_ = azimuthLatch->isRelatching();
    azLatchChannel_ = "";

    latch_ = Utils::degreesToRadians(azLatchAzimuth_);
    caught_ = false;
    latchChannel_ = 0;
}

void
HistoryPosition::liveFrameChanged()
{
    static Logger::ProcLog log("liveFrameChanged", Log());
    LOGINFO << std::endl;
    if (!viewingPast_) {
        bool show = checkAzimuthLatch();

        if (show) {
            if (caught_) {
                LOGDEBUG << "caught azimuth" << std::endl;
                pastFrame_ = history_.getLiveFrame();
            }

            emit viewChanged();

            if (infoReporter_) { emit infoUpdate(history_.getLiveFrame().getSomeMessage()); }
        }
    }
}

void
HistoryPosition::setInfoReporter(bool state)
{
    if (infoReporter_ == state) return;
    infoReporter_ = state;
    if (state) {
        if (caught_ || viewingPast_)
            emit infoUpdate(pastFrame_.getSomeMessage());
        else
            emit infoUpdate(history_.getLiveFrame().getSomeMessage());
        azimuthLatch_->setConfiguration(azLatchEnabled_, azLatchAzimuth_, azLatchRelatch_, caught_, names_,
                                        azLatchChannel_);
    }
}

void
HistoryPosition::pastFrozen()
{
    static Logger::ProcLog log("pastFrozen", Log());
    LOGINFO << std::endl;

    // The past is being frozen. Initialize our viewing position to the most recent entry. NOTE: viewFrozen_
    // could be true if we overrode a thaw in the pastThawed() slot, in which case we do not want to change the
    // position value.
    //
    if (!viewingPast_) position_ = 0;
}

void
HistoryPosition::pastThawed()
{
    static Logger::ProcLog log("pastThawed", Log());
    LOGINFO << std::endl;

    // Someone asked that the past be thawed. However, if we are still viewing the past, override the request.
    //
    if (viewingPast_) {
        history_.freezePast(true);
    } else {
        position_ = 0;
    }
}

void
HistoryPosition::setPosition(int position)
{
    static Logger::ProcLog log("setPosition", Log());
    LOGINFO << position << std::endl;
    if (!viewingPast_) return;
    if (position == position_) return;
    if (sender() && !synchronized_) return;
    position_ = position;
    pastFrame_ = history_.getPastFrame(position_);
    emit viewChanged();
    if (infoReporter_) emit infoUpdate(pastFrame_.getSomeMessage());
}

void
HistoryPosition::viewingPast(bool state)
{
    if (viewingPast_ != state) {
        viewingPast_ = state;
        history_.freezePast(state);
        if (state) pastFrame_ = history_.getPastFrame(position_);
        emit viewChanged();
        emit viewingPastChanged(state);
        if (infoReporter_) emit infoUpdate(pastFrame_.getSomeMessage());
    }
}

Messages::PRIMessage::Ref
HistoryPosition::getMessage(int slot) const
{
    static Logger::ProcLog log("getMessage", Log());
    LOGINFO << "position: " << position_ << " slot: " << slot << std::endl;
    return (caught_ || viewingPast_) ? pastFrame_.getMessage(history_.getSlotIndex(slot))
                                     : history_.getLiveMessage(slot);
}

void
HistoryPosition::setSynchronized(bool synchronized)
{
    synchronized_ = synchronized;
}

bool
HistoryPosition::checkAzimuthLatch()
{
    static Logger::ProcLog log("checkAzimuthLatch", Log());

    if (!latchChannel_) return true;

    int slot = latchChannel_->getHistorySlot();
    if (slot == -1) return true;

    Messages::PRIMessage::Ref msg(history_.getLiveMessage(slot));

    double azimuth = msg->getAzimuthStart();

    if (!azLatchEnabled_ || lastAzimuth_ < 0.0) {
        lastAzimuth_ = azimuth;
        return true;
    }

    bool caught = false;

    if (azimuth == lastAzimuth_) { return !caught_; }

    if (azimuth > lastAzimuth_) {
        caught = lastAzimuth_ < latch_ && azimuth >= latch_;
        LOGDEBUG << "test 1: " << caught << std::endl;
    } else {
        caught = (lastAzimuth_ < latch_ && azimuth + Utils::kCircleRadians >= latch_) ||
                 (lastAzimuth_ - Utils::kCircleRadians < latch_ && azimuth >= latch_);
        LOGDEBUG << "test 2: " << caught << std::endl;
    }

    lastAzimuth_ = azimuth;

    if (caught) {
        LOGDEBUG << "caught " << azimuth << ' ' << msg->getSequenceCounter() << std::endl;

        // Caught an azimuth.
        //
        if (!caught_) {
            caught_ = true;
            if (infoReporter_) azimuthLatch_->latch();
            return true;
        }

        // If relatching is enabled, let this one thru.
        //
        if (azLatchRelatch_) {
            LOGDEBUG << "relatched" << std::endl;
            return true;
        }
    }

    return !caught_;
}

void
HistoryPosition::azimuthLatchChanged(bool enabled, double azimuth, bool relatching, const QString& name)
{
    if (!infoReporter_) return;

    if (azLatchEnabled_ != enabled) {
        caught_ = false;
        lastAzimuth_ = -1.0;
        azimuthLatch_->reset();
    }

    if (!name.isEmpty()) {
        if (name != azLatchChannel_) {
            azLatchChannel_ = name;
            caught_ = false;
            lastAzimuth_ = -1.0;
            azimuthLatch_->reset();
        }

        if (!latchChannel_) latchChannel_ = parent_->getChannelConnection(azLatchChannel_);
    }

    if (azLatchAzimuth_ != azimuth) {
        azLatchAzimuth_ = azimuth;
        latch_ = Utils::degreesToRadians(azimuth);
    }

    azLatchEnabled_ = enabled;
    azLatchRelatch_ = relatching;
}

void
HistoryPosition::saveToSettings(QSettings& settings)
{
    Logger::ProcLog log("saveToSettings", Log());
    LOGINFO << "azLatchChannel: " << azLatchChannel_ << std::endl;
    settings.setValue(kAzimuthLatchAzimuth, azLatchAzimuth_);
    settings.setValue(kAzimuthLatchRelatch, azLatchRelatch_);
    settings.setValue(kAzimuthLatchChannel, azLatchChannel_);
}

void
HistoryPosition::restoreFromSettings(QSettings& settings)
{
    Logger::ProcLog log("restoreFromSettings", Log());
    azLatchEnabled_ = false;
    azLatchAzimuth_ = settings.value(kAzimuthLatchAzimuth, 0.0).toDouble();
    azLatchRelatch_ = settings.value(kAzimuthLatchRelatch, true).toBool();
    azLatchChannel_ = settings.value(kAzimuthLatchChannel, "").toString();
    LOGINFO << "azLatchChannel: " << azLatchChannel_ << std::endl;
}

void
HistoryPosition::channelNamesChanged(const QSet<QString>& names)
{
    if (!names.contains(azLatchChannel_)) {
        latchChannel_ = 0;
        caught_ = false;
    }
    names_ = names.values();
    azimuthLatch_->setConfiguration(azLatchEnabled_, azLatchAzimuth_, azLatchRelatch_, caught_, names_,
                                    azLatchChannel_);
}
