#include "QtCore/QSettings"
#include "QtCore/QTimer"

#include "GUI/MessageList.h"
#include "GUI/ServiceEntry.h"
#include "GUI/LogUtils.h"
#include "IO/MessageManager.h"
#include "Messages/Header.h"
#include "Messages/MetaTypeInfo.h"

#include "App.h"
#include "DefaultViewSettings.h"
#include "VideoChannel.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::AScope;

static const char* const kColor = "Color";
static const char* const kSampleMin = "SampleMin";
static const char* const kSampleMax = "SampleMax";
static const char* const kVoltageMin = "VoltageMin";
static const char* const kVoltageMax = "VoltageMax";

Logger::Log&
VideoChannel::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.VideoChannel");
    return log_;
}

VideoChannel::VideoChannel(History& history, const QString& name)
    : QObject(), videoInfo_(Messages::Video::GetMetaTypeInfo()),
      history_(history), peakBars_(), historySlot_(size_t(-1)),
      reader_(0), name_(name), color_(), displayCount_(0)
{
    static Logger::ProcLog log("VideoChannel", Log());
    LOGINFO << "name: " << name << std::endl;
}

VideoChannel::~VideoChannel()
{
    static Logger::ProcLog log("~VideoChannel", Log());
    LOGINFO << std::endl;
    if (historySlot_ != size_t(-1))
	history_.releaseSlot(historySlot_);
}

size_t
VideoChannel::displayAdded()
{
    Logger::ProcLog log("displayAdded", Log());
    LOGINFO << name_ << ' ' << displayCount_ << std::endl;
    if (displayCount_++ == 0) {
	historySlot_ = history_.allocateSlot();
    }

    return historySlot_;
}

void
VideoChannel::displayDropped()
{
    Logger::ProcLog log("displayDropped", Log());
    LOGINFO << name_ << ' ' << displayCount_ << std::endl;
    if (--displayCount_ == 0) {
	shutdown();
	history_.releaseSlot(historySlot_);
	historySlot_ = size_t(-1);
    }
}

bool
VideoChannel::isConnected() const
{
    return reader_ && reader_->isConnected();
}

void
VideoChannel::shutdown()
{
    Logger::ProcLog log("shutdown", Log());
    LOGINFO << name_ << std::endl;
    if (reader_) {
	delete reader_;
	reader_ = 0;
    }
    LOGINFO << name_ << " done" << std::endl;
}

void
VideoChannel::setColor(const QColor& color)
{
    Logger::ProcLog log("setColor", Log());
    LOGDEBUG << color.name() << std::endl;
    if (color_ != color) {
	color_ = color;
	emit colorChanged(color);
    }
}

void
VideoChannel::readerIncoming()
{
    Logger::ProcLog log("readerIncoming", Log());
    LOGINFO << name_ << std::endl;
    const MessageList& messages(reader_->getMessages());
    if (! messages.empty()) {
	history_.update(historySlot_, messages);
	peakBars_.update(history_.getLiveMessage(historySlot_));
    }
}

void
VideoChannel::useServiceEntry(ServiceEntry* serviceEntry)
{
    static Logger::ProcLog log("useServiceEntry", Log());
    LOGERROR << name_ << ' ' << serviceEntry << std::endl;

    if (! reader_) {
	reader_ = new Subscriber(this);
	connect(reader_, SIGNAL(connected()), SIGNAL(connected()));
	connect(reader_, SIGNAL(disconnected()), SIGNAL(disconnected()));
	connect(reader_, SIGNAL(dataAvailable()), SLOT(readerIncoming()));
    }

    reader_->useServiceEntry(serviceEntry);
}

void
VideoChannel::setSampleToVoltageScaling(int sampleMin, int sampleMax,
                                        double voltageMin, double voltageMax)
{
    sampleMin_ = sampleMin;
    sampleMax_ = sampleMax;
    voltageMin_ = voltageMin;
    voltageMax_ = voltageMax;
    if (sampleMax != sampleMin)
	voltageScale_ = (voltageMax - voltageMin) / (sampleMax - sampleMin);
    else
	voltageScale_ = 0.0;

    emit sampleToVoltageScalingChanged();
}

void
VideoChannel::restoreFromSettings(QSettings& settings)
{
    settings.beginGroup(name_);
    color_ = settings.value(kColor, QColor()).value<QColor>();

    if (! color_.isValid()) {
	while (1) {
	    int red = int(double(::random()) / 2147483648.0 * 256);
	    int green = int(double(::random()) / 2147483648.0 * 256);
	    int blue = int(double(::random()) / 2147483648.0 * 256);
	    if (red < 128 && green < 128 && blue < 128)
		continue;

	    color_ = QColor(red, green, blue);
	    break;
	}
    }

    DefaultViewSettings& viewSettings(
	App::GetApp()->getDefaultViewSettings());

    setSampleToVoltageScaling(
	settings.value(kSampleMin, viewSettings.getSampleMin()).toInt(),
	settings.value(kSampleMax, viewSettings.getSampleMax()).toInt(),
	settings.value(kVoltageMin, viewSettings.getVoltageMin()).toDouble(),
	settings.value(kVoltageMax,
                       viewSettings.getVoltageMax()).toDouble());
    settings.endGroup();
}

void
VideoChannel::saveToSettings(QSettings& settings) const
{
    settings.beginGroup(name_);
    settings.setValue(kColor, color_);
    settings.setValue(kSampleMin, sampleMin_);
    settings.setValue(kSampleMax, sampleMax_);
    settings.setValue(kVoltageMin, voltageMin_);
    settings.setValue(kVoltageMax, voltageMax_);
    settings.endGroup();
}
