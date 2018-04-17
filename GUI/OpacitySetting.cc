#include "QtWidgets/QSlider"
#include "QtWidgets/QSpinBox"

#include "AppBase.h"
#include "LogUtils.h"
#include "OpacitySetting.h"

using namespace SideCar::GUI;

Logger::Log&
OpacitySetting::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.OpacitySetting");
    return log_;
}

OpacitySetting::OpacitySetting(PresetManager* mgr, QSpinBox* widget, bool global) :
    DoubleSetting(mgr, widget->objectName(), FromWidget(widget->value()), global)
{
    connectWidget(widget);
}

void
OpacitySetting::connectWidget(QSlider* widget)
{
    widget->setValue(ToWidget(getValue()));
    connect(widget, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));
    connect(this, SIGNAL(percentageChanged(int)), widget, SLOT(setValue(int)));
    Setting::connectWidget(widget);
}

void
OpacitySetting::connectWidget(QSpinBox* widget)
{
    widget->setKeyboardTracking(false);
    widget->setSuffix("%");
    widget->setValue(ToWidget(getValue()));
    connect(widget, SIGNAL(valueChanged(int)), this, SLOT(widgetChanged(int)));
    connect(this, SIGNAL(percentageChanged(int)), widget, SLOT(setValue(int)));
    Setting::connectWidget(widget);
}

void
OpacitySetting::widgetChanged(int value)
{
    setValue(FromWidget(value));
}

void
OpacitySetting::sliderChanged(int value)
{
    qobject_cast<QWidget*>(sender())->setToolTip(QString("%1%").arg(value));
    widgetChanged(value);
}

void
OpacitySetting::valueUpdated()
{
    DoubleSetting::valueUpdated();
    emit percentageChanged(ToWidget(getValue()));
}
