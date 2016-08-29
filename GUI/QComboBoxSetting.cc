#include "GUI/LogUtils.h"

#include "QComboBoxSetting.h"

using namespace SideCar::GUI;

Logger::Log&
QComboBoxSetting::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.GUI.QComboBoxSetting");
    return log_;
}

QComboBoxSetting::QComboBoxSetting(PresetManager* mgr, QComboBox* widget,
                                   bool global)
    : Super(mgr, widget->objectName(), widget->currentIndex(), global),
      first_(widget)
{
    connectWidget(widget);
}

void
QComboBoxSetting::connectWidget(QComboBox* widget)
{
    widget->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    Super::connectWidget(widget);
}

QComboBox*
QComboBoxSetting::duplicate(QWidget* parent)
{
    Logger::ProcLog log("duplicate", Log());
    LOGINFO << "parent: " << parent << " first: " << first_ << std::endl;
    QComboBox* widget = new QComboBox(parent);
    widget->setModel(first_->model());
    widget->setCurrentIndex(first_->currentIndex());
    connectWidget(widget);
    return widget;
}
