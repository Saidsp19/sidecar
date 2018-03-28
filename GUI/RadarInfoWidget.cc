#include "QtCore/QDateTime"

#include "Logger/Log.h"

#include "AppBase.h"
#include "RadarInfoWidget.h"

#include "ui_RadarInfoWidget.h"

using namespace SideCar::GUI;

const int RadarInfoWidget::kUpdateInterval = 250;

Logger::Log&
RadarInfoWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.RadarInfoWidget");
    return log_;
}

RadarInfoWidget::RadarInfoWidget(QWidget* parent) :
    Super(parent), gui_(new Ui::RadarInfoWidget), app_(AppBase::GetApp()), updateTimer_(), irigTime_(0.0),
    createdTime_(0.0), azimuth_(0.0), hasIRIGTime_(true), updateTime_(true), updateAzimuth_(true)
{
    gui_->setupUi(this);
    gui_->value_->setTextFormat(Qt::PlainText);
    connect(AppBase::GetApp(), SIGNAL(angleFormattingChanged(int)), SLOT(refreshAzimuth()));
    updateTimer_.start(kUpdateInterval, this);
    refresh();
}

void
RadarInfoWidget::showMessageInfo(const Messages::PRIMessage::Ref& msg)
{
    if (!msg) return;

    if (msg->hasIRIGTime()) {
        if (!hasIRIGTime_) {
            hasIRIGTime_ = true;
            updateTime_ = true;
        }

        if (irigTime_ != msg->getRIUInfo().irigTime) {
            irigTime_ = msg->getRIUInfo().irigTime;
            updateTime_ = true;
        }
    } else {
        if (hasIRIGTime_) {
            hasIRIGTime_ = false;
            updateTime_ = true;
        }

        if (createdTime_ != msg->getCreatedTimeStamp()) {
            createdTime_ = msg->getCreatedTimeStamp();
            updateTime_ = true;
        }
    }

    if (azimuth_ != msg->getAzimuthStart()) {
        azimuth_ = msg->getAzimuthStart();
        updateAzimuth_ = true;
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

    if (updateAzimuth_) {
        updateAzimuth_ = false;
        azimuthText_ = QString("Az: ") + app_->getFormattedAngleRadians(azimuth_);
    }

    if (updateTime_) {
        updateTime_ = false;
        if (hasIRIGTime_) {
            time_t when = static_cast<time_t>(::floor(irigTime_));
            QDateTime dateTime;
            dateTime.setTime_t(when);
            timeText_ = QString(" IRIG: ") + dateTime.toString("hh:mm:ss UTC");
        } else {
            timeText_ = QString(" Local: ") + QString::fromStdString(createdTime_.hhmmss(2));
        }
    }

    return azimuthText_ + timeText_;
}

void
RadarInfoWidget::timerEvent(QTimerEvent* event)
{
    static Logger::ProcLog log("timerEvent", Log());

    if (event->timerId() == updateTimer_.timerId()) {
        if (needUpdate()) { refresh(); }
    } else {
        Super::timerEvent(event);
    }
}

bool
RadarInfoWidget::needUpdate() const
{
    return updateTime_ || updateAzimuth_;
}

void
RadarInfoWidget::refreshTime()
{
    updateTime_ = true;
    refresh();
}

void
RadarInfoWidget::refreshAzimuth()
{
    updateAzimuth_ = true;
    refresh();
}
