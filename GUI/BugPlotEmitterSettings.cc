#include "QtCore/QStringList"
#include "QtCore/QTimer"
#include "QtNetwork/QHostInfo"

#include "IO/MessageManager.h"
#include "IO/ZeroconfRegistry.h"
#include "Messages/BugPlot.h"

#include "AppBase.h"
#include "BugPlotEmitterSettings.h"
#include "ChannelSetting.h"
#include "LogUtils.h"
#include "Writers.h"

using namespace SideCar;
using namespace SideCar::GUI;

Logger::Log&
BugPlotEmitterSettings::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.BugPlotEmitterSettings");
    return log_;
}

BugPlotEmitterSettings::BugPlotEmitterSettings(BoolSetting* enabled, StringSetting* serviceName,
                                               StringSetting* address, ChannelSetting* channel)
    : Super(enabled), serviceName_(serviceName), address_(address), channel_(channel), writer_(0),
      localHostName_(QHostInfo::localHostName()), sequenceCounter_(0), needUpdate_(false)
{
    Logger::ProcLog log("BugPlotEmitterSettings", Log());
    localHostName_ = localHostName_.split('.')[0];
    LOGINFO << "localHostName: " << localHostName_ << std::endl;
    add(serviceName);
    add(address);
    connect(this, SIGNAL(settingChanged()), SLOT(needUpdate()));
    connect(this, SIGNAL(enabledChanged(bool)), SLOT(needUpdate()));
    updateWriter();
}

BugPlotEmitterSettings::~BugPlotEmitterSettings()
{
    delete writer_;
}

void
BugPlotEmitterSettings::needUpdate()
{
    if (! needUpdate_) {
	needUpdate_ = true;
	QTimer::singleShot(0, this, SLOT(updateWriter()));
    }
}

void
BugPlotEmitterSettings::updateWriter()
{
    needUpdate_ = false;
    if (isEnabled())
	addWriter();
    else
	removeWriter();
}

void
BugPlotEmitterSettings::addWriter()
{
    Logger::ProcLog log("addWriter", Log());
    LOGINFO << std::endl;

    removeWriter();
    QString serviceName = QString("%1 %2").arg(serviceName_->getValue()) .arg(localHostName_);
    std::string subType = Messages::BugPlot::GetMetaTypeInfo().getName();
    LOGDEBUG << "serviceName: " << serviceName << " subType: " << subType << std::endl;
    writer_ = UDPMessageWriter::Make(serviceName, subType, address_->getValue());
}

void
BugPlotEmitterSettings::removeWriter()
{
    Logger::ProcLog log("addWriter", Log());
    LOGINFO << std::endl;
    delete writer_;
    writer_ = 0;
}

Messages::BugPlot::Ref
BugPlotEmitterSettings::addBugPlot(double range, double azimuth, double elevation)
{
    Logger::ProcLog log("addBugPlot", Log());
    LOGTIN << "range: " << range << " azimuth: " << azimuth
	   << " elevation: " << elevation
	   << " writer: " << (writer_ ? 'Y' : 'N') << std::endl;

    // Create a new BugPlot message to send out.
    //
    QString tag = QString("%1/%2").arg(localHostName_).arg(++sequenceCounter_);
    Time::TimeStamp now = Time::TimeStamp::Now();
    Messages::BugPlot::Ref msg = Messages::BugPlot::Make(AppBase::GetApp()->getApplicationName().toStdString(),
                                                         now.asDouble(), range, azimuth, elevation,
                                                         tag.toStdString());

    // If the writer is not active, let the caller handle the new message.
    //
    if (! writer_) return msg;

    // Send out.
    //
    IO::MessageManager mgr(msg);
    writer_->writeMessage(mgr);

    // If the user is connected to a BugPlot channel, then we zap the message reference since it was properly
    // handled.
    //
    if (channel_->isConnected())
	msg.reset();

    return msg;
}
