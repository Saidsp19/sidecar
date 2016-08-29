#include "StatusWidget.h"

using namespace SideCar::GUI::Master;

StatusWidget::StatusWidget(QWidget* parent)
    : QWidget(parent), Ui::StatusWidget()
{
    setupUi(this);
    QString zero(QString::number(0));
    activeMasters_->setText(zero);
    activeConfigs_->setText(zero);
    activeRunners_->setText(zero);
    activeStreams_->setText(zero);
    failures_->setText(zero);
}
