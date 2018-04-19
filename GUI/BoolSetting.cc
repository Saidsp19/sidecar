#include "QtWidgets/QAction"
#include "QtWidgets/QCheckBox"
#include "QtWidgets/QRadioButton"

#include "BoolSetting.h"
#include "Utils.h"

using namespace SideCar::GUI;

BoolSetting::BoolSetting(PresetManager* mgr, const QString& name, bool global) : Super(mgr, name, false, global)
{
    value_ = false;
}

BoolSetting::BoolSetting(PresetManager* mgr, const QString& name, bool value, bool global) :
    Super(mgr, name, value, global)
{
    value_ = value;
}

void
BoolSetting::setValue(bool value)
{
    value_ = value;
    setOpaqueValue(value);
}

void
BoolSetting::valueUpdated()
{
    value_ = getOpaqueValue().toBool();
    emit valueChanged(value_);
}

void
BoolSetting::connectAction(QAction* action)
{
    action->setCheckable(true);
    action->setChecked(getValue());
    connect(action, SIGNAL(triggered(bool)), this, SLOT(setValue(bool)));
    connect(this, SIGNAL(valueChanged(bool)), action, SLOT(setChecked(bool)));
    connect(action, SIGNAL(triggered()), this, SLOT(updateAction()));
    UpdateToggleAction(action, action->isChecked());
}

void
BoolSetting::updateAction()
{
    QAction* action = dynamic_cast<QAction*>(sender());
    if (action) { UpdateToggleAction(action, action->isChecked()); }
}

void
BoolSetting::connectWidget(QCheckBox* widget)
{
    widget->setChecked(getValue());
    connect(widget, SIGNAL(toggled(bool)), SLOT(setValue(bool)));
    connect(this, SIGNAL(valueChanged(bool)), widget, SLOT(setChecked(bool)));
    Super::connectWidget(widget);
}

void
BoolSetting::connectWidget(QRadioButton* widget)
{
    widget->setChecked(getValue());
    connect(widget, SIGNAL(toggled(bool)), this, SLOT(setValue(bool)));
    connect(this, SIGNAL(valueChanged(bool)), widget, SLOT(setChecked(bool)));
    Super::connectWidget(widget);
}
