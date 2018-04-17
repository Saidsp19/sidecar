#include "QtWidgets/QLineEdit"

#include "QLineEditSetting.h"

using namespace SideCar::GUI;

QLineEditSetting::QLineEditSetting(PresetManager* mgr, QLineEdit* widget, bool global) :
    StringSetting(mgr, widget->objectName(), widget->text(), global)
{
    widget->setText(getValue());
    connect(widget, SIGNAL(editingFinished()), this, SLOT(widgetChanged()));
    connect(this, SIGNAL(valueChanged(const QString&)), widget, SLOT(setText(const QString&)));
}

void
QLineEditSetting::widgetChanged()
{
    setValue(static_cast<QLineEdit*>(sender())->text());
}
