#include "QtCore/QDateTime"

#include "Logger/Log.h"

#include "App.h"
#include "Configuration.h"
#include "RadarInfoWidget.h"
#include "RadarSettings.h"

#include "ui_RadarInfoWidget.h"

using namespace SideCar::GUI::ESScope;

const int RadarInfoWidget::kUpdateInterval = 250;

Logger::Log&
RadarInfoWidget::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("esscope.RadarInfoWidget");
    return log_;
}

RadarInfoWidget::RadarInfoWidget(QWidget* parent)
    : Super(parent), gui_(new Ui::RadarInfoWidget),
      app_(App::GetApp()),
      radarSettings_(app_->getConfiguration()->getRadarSettings()),
      updateTimer_(), alpha_(0.0), beta_(0.0), irigTime_(0.0),
      hasIRIGTime_(true), updateAlpha_(true), updateBeta_(true),
      updateTime_(true)
{
    gui_->setupUi(this);
    gui_->value_->setTextFormat(Qt::PlainText);
    setTilt(radarSettings_->getTilt());
    setRotation(radarSettings_->getRotation());
    connect(AppBase::GetApp(), SIGNAL(angleFormattingChanged(int)),
            SLOT(refreshTilt()));
    connect(AppBase::GetApp(), SIGNAL(angleFormattingChanged(int)),
            SLOT(refreshRotation()));
    connect(radarSettings_, SIGNAL(tiltChanged(double)),
            SLOT(setTilt(double)));
    connect(radarSettings_, SIGNAL(rotationChanged(double)),
            SLOT(setRotation(double)));
    updateTimer_.start(kUpdateInterval, this);
    refresh();
}

void
RadarInfoWidget::showMessageInfo(const Messages::PRIMessage::Ref& msg)
{
    if (! msg) return;

    double alpha = radarSettings_->getAlpha(msg);
    if (alpha_ != alpha) {
	alpha_ = alpha;
	updateAlpha_ = true;
    }
    
    double beta = radarSettings_->getBeta(msg);
    if (beta_ != beta) {
	beta_ = beta;
	updateBeta_ = true;
    }

    if (msg->hasIRIGTime()) {
	if (! hasIRIGTime_) {
	    hasIRIGTime_ = true;
	    updateTime_ = true;
	}

	if (irigTime_ != msg->getRIUInfo().irigTime) {
	    irigTime_ = msg->getRIUInfo().irigTime;
	    updateTime_ = true;
	}
    }
    else  {
	if (hasIRIGTime_) {
	    hasIRIGTime_ = false;
	    updateTime_ = true;
	}

	if (createdTime_ != msg->getCreatedTimeStamp()) {
	    createdTime_ = msg->getCreatedTimeStamp();
	    updateTime_ = true;
	}
    }
}

void
RadarInfoWidget::refresh()
{
    static Logger::ProcLog log("refresh", Log());
    LOGINFO << std::endl;
    gui_->value_->setText(makeLabel());
    updateGeometry();
}

QString
RadarInfoWidget::makeLabel()
{
    static Logger::ProcLog log("makeLabel", Log());
    LOGINFO << std::endl;

    if (updateAlpha_) {
	updateAlpha_ = false;
	alphaText_ = QString("Alpha: %1").arg(alpha_, 6, 'f', 3);
    }

    if (updateBeta_) {
	updateBeta_ = false;
	betaText_ = QString("  Beta: %1").arg(beta_, 6, 'f', 3);
    }

    if (updateTime_) {
	updateTime_ = false;
	timeText_ = "  Time: ";
	if (hasIRIGTime_) {
	    time_t when = static_cast<time_t>(::floor(irigTime_));
	    QDateTime dateTime;
	    dateTime.setTime_t(when);
	    timeText_ += dateTime.toString("hh:mm:ss UTC");
	}
	else {
	    timeText_ += QString::fromStdString(createdTime_.hhmmss(2));
	}
    }

    return alphaText_ + betaText_ + timeText_ + tiltText_ + rotationText_;
}

void
RadarInfoWidget::refreshTilt()
{
    setTilt(radarSettings_->getTilt());
}

void
RadarInfoWidget::setTilt(double value)
{
    tiltText_ = QString("  Tilt: %1").arg(
	app_->getFormattedAngleDegrees(value));
    refresh();
}

void
RadarInfoWidget::refreshRotation()
{
    setRotation(radarSettings_->getRotation());
}

void
RadarInfoWidget::setRotation(double value)
{
    rotationText_ = QString("  Rot: %1").arg(
	app_->getFormattedAngleDegrees(value));
    refresh();
}

void
RadarInfoWidget::timerEvent(QTimerEvent* event)
{
    static Logger::ProcLog log("timerEvent", Log());

    if (event->timerId() == updateTimer_.timerId()) {
	if (updateAlpha_ || updateBeta_ || updateTime_) {
	    refresh();
	}
    }
    else {
	Super::timerEvent(event);
    }
}
