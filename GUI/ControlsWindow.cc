#include "Logger/Log.h"

#include "ControlsWindow.h"
#include "VideoSampleCountTransform.h"
#include "ui_ControlsWindow.h"

using namespace SideCar::GUI;

Logger::Log&
ControlsWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.ControlsWindow");
    return log_;
}

ControlsWindow::ControlsWindow(int shortcut) : ToolWindowBase("ControlsWindow", "Controls", shortcut)
{
    Ui::ControlsWindow gui;
    gui.setupUi(this);

    gain_ = gui.gain_;
    gainValue_ = gui.gainValue_;

    cutoffMin_ = gui.cutoffMin_;
    cutoffMinValue_ = gui.cutoffMinValue_;
    cutoffMinMin_ = gui.cutoffMinMin_;
    cutoffMinMax_ = gui.cutoffMinMax_;

    cutoffMax_ = gui.cutoffMax_;
    cutoffMaxValue_ = gui.cutoffMaxValue_;
    cutoffMaxMin_ = gui.cutoffMaxMin_;
    cutoffMaxMax_ = gui.cutoffMaxMax_;

    connect(gain_, SIGNAL(valueChanged(int)), SLOT(gainValueChanged(int)));
    connect(cutoffMin_, SIGNAL(valueChanged(int)), SLOT(cutoffMinValueChanged(int)));
    connect(cutoffMax_, SIGNAL(valueChanged(int)), SLOT(cutoffMaxValueChanged(int)));
}

void
ControlsWindow::showEvent(QShowEvent* event)
{
    setFixedHeight(sizeHint().height());
    Super::showEvent(event);
}

void
ControlsWindow::setVideoSampleCountTransform(const VideoSampleCountTransform* transform)
{
    transform_ = transform;
    connect(transform_, SIGNAL(settingChanged()), SLOT(update()));
    update();
}

void
ControlsWindow::gainValueChanged(int value)
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
ControlsWindow::cutoffMinValueChanged(int value)
{
    Logger::ProcLog log("cutoffMinValueChanged", Log());
    LOGINFO << "value: " << value << std::endl;
    QString tip = QString::number(value);
    cutoffMin_->setToolTip(tip);
    cutoffMin_->setStatusTip(tip);
}

void
ControlsWindow::cutoffMaxValueChanged(int value)
{
    Logger::ProcLog log("cutoffMaxValueChanged", Log());
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
