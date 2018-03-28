#include "GUI/LogUtils.h"
#include "GUI/VideoSampleCountTransform.h"

#include "Configuration.h"
#include "ControlsWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::BScope;

Logger::Log&
ControlsWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.ControlsWindow");
    return log_;
}

ControlsWindow::ControlsWindow(int shortcut) :
    ToolWindowBase("ControlsWindow", "Controls", shortcut), Ui::ControlsWindow()
{
    setupUi(this);
    setFixedHeight(sizeHint().height());
}

void
ControlsWindow::setConfiguration(const Configuration* configuration)
{
    transform_ = configuration->getVideoSampleCountTransform();
    update();
    connect(transform_, SIGNAL(settingChanged()), SLOT(update()));
}

void
ControlsWindow::on_gain__valueChanged(int value)
{
    gain_->setValue(value);

    float gain = 1.0 + value * 0.01;

    QString tip;
    if (gain < 1.0)
        tip = QString("%1%").arg(int(gain * 100));
    else
        tip = QString("x%1").arg(gain);
    gainValue_->setText(tip);
    gain_->setToolTip(tip);
    tip.prepend("Gain: ");
    gain_->setStatusTip(tip);
}

void
ControlsWindow::on_cutoffMin__valueChanged(int value)
{
    Logger::ProcLog log("on_cutoffMin__valueChanged", Log());
    LOGINFO << "value: " << value << std::endl;
    QString tip = QString::number(value);
    cutoffMin_->setToolTip(tip);
    cutoffMin_->setStatusTip(tip);
}

void
ControlsWindow::on_cutoffMax__valueChanged(int value)
{
    Logger::ProcLog log("on_cutoffMax__valueChanged", Log());
    LOGINFO << "value: " << value << std::endl;
    QString tip = QString::number(value);
    cutoffMax_->setToolTip(tip);
    cutoffMax_->setStatusTip(tip);
}

void
ControlsWindow::update()
{
    static Logger::ProcLog log("update", Log());

    int sampleMax = transform_->getSampleMax();
    int sampleMin = transform_->getSampleMin();

    LOGINFO << "sampleMin: " << sampleMin << " sampleMax: " << sampleMax << std::endl;
    LOGINFO << "cutoffMin: " << cutoffMin_->value() << " cutoffMax: " << cutoffMax_->value() << std::endl;

    cutoffMin_->setRange(sampleMin, sampleMax);
    cutoffMax_->setRange(sampleMin, sampleMax);

    cutoffMin_->setMaximum(cutoffMax_->value() - 1);
    cutoffMax_->setMinimum(cutoffMin_->value() + 1);

    cutoffMinMin_->setText(QString::number(sampleMin));
    cutoffMinMax_->setText(QString::number(cutoffMin_->maximum()));

    cutoffMaxMin_->setText(QString::number(cutoffMax_->minimum()));
    cutoffMaxMax_->setText(QString::number(sampleMax));

    cutoffMinValue_->setText(QString::number(cutoffMin_->value()));
    cutoffMaxValue_->setText(QString::number(cutoffMax_->value()));
}
