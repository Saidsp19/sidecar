#include "UtilsWidget.h"

using namespace SideCar::GUI::Master;

UtilsWidget::UtilsWidget(QWidget* parent) : QWidget(parent), Ui::UtilsWidget()
{
    setupUi(this);
    runStateLabel_->setForegroundRole(QPalette::WindowText);
    logsLabel_->setForegroundRole(QPalette::WindowText);
}
