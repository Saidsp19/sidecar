#include "QtCore/QEvent"
#include "QtWidgets/QInputDialog"
#include "QtWidgets/QStatusBar"

#include "LogUtils.h"
#include "QSliderSetting.h"

using namespace SideCar::GUI;

Logger::Log&
QSliderSetting::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.QSliderSetting");
    return log_;
}

QSliderSetting::QSliderSetting(PresetManager* mgr, QSlider* widget, bool global) :
    Super(mgr, widget->objectName(), widget->value(), global), sliders_(), updatingRanges_(false)
{
    Logger::ProcLog log("QSliderSetting", Log());
    LOGINFO << "name: " << getName() << " value: " << getValue() << std::endl;
    widget->setValue(getValue());
    connectWidget(widget);
}

void
QSliderSetting::connectWidget(QSlider* widget)
{
    Logger::ProcLog log("connectWidget", Log());

    sliders_.append(widget);
    if (sliders_.size() > 1) {
        QSlider* first = sliders_[0];

        // Connect a 'slave' QSlider to this setting. NOTE: do the following *before* connecting signals. The
        // alternative would be to block all signals from the 'slave' widget.
        //
        widget->installEventFilter(this);
        widget->setPageStep(first->pageStep());
        widget->setRange(first->minimum(), first->maximum());
        widget->setValue(first->value());
    }

    connect(widget, SIGNAL(valueChanged(int)), SLOT(setValue(int)));

    connect(widget, SIGNAL(rangeChanged(int, int)), SLOT(rangeChanged(int, int)));

    Setting::connectWidget(widget);

    QString tip = QString::number(getValue());
    LOGINFO << "name: " << getName() << " toolTip: " << tip << std::endl;

    widget->setToolTip(makeToolTip());
}

void
QSliderSetting::rangeChanged(int min, int max)
{
    static Logger::ProcLog log("rangeChanged", Log());
    LOGINFO << "name: " << getName() << " min: " << min << " max: " << max << std::endl;

    if (updatingRanges_ || sliders_.empty()) return;

    updatingRanges_ = true;
    for (int index = 0; index < sliders_.size(); ++index) {
        QSlider* slider = sliders_[index];
        if (slider->minimum() != min || slider->maximum() != max) { slider->setRange(min, max); }
    }

    updatingRanges_ = false;
}

QString
QSliderSetting::makeToolTip() const
{
    return QString::number(getValue());
}

bool
QSliderSetting::eventFilter(QObject* object, QEvent* event)
{
    QSlider* slider = qobject_cast<QSlider*>(object);

    if (!slider) return Super::eventFilter(object, event);

    if (event->type() == QEvent::MouseButtonPress) {
        valueAtPress_ = slider->value();
        return false;
    }

    if (event->type() != QEvent::MouseButtonDblClick) return false;

    if (valueAtPress_ != slider->value()) slider->setValue(valueAtPress_);

    bool ok;
    int value = QInputDialog::getInt(0, "Slider Value", "New value: ", valueAtPress_, slider->minimum(),
                                     slider->maximum(), slider->singleStep(), &ok);
    if (!ok) return true;

    slider->setValue(value);

    return true;
}

void
QSliderSetting::valueUpdated()
{
    Logger::ProcLog log("valueUpdated", Log());

    Super::valueUpdated();
    int value = getValue();

    LOGINFO << "name: " << getName() << " value: " << value << std::endl;

    QString tip = makeToolTip();
    for (int index = 0; index < sliders_.size(); ++index) {
        QSlider* slider = sliders_[index];
        if (value < slider->minimum()) slider->setMinimum(value);
        if (value > slider->maximum()) slider->setMaximum(value);
        slider->setValue(value);
        slider->setToolTip(tip);
    }
}

double
QSliderSetting::getNormalizedValue() const
{
    QSlider* slider = sliders_[0];
    return double(getValue() - slider->minimum()) / double(slider->maximum() - slider->minimum());
}
