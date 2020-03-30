#include "QtGui/QGuiApplication"

#include "StatusWidget.h"

using namespace SideCar::GUI::Master;

StatusWidget::StatusWidget(QWidget* parent) : QWidget(parent), Ui::StatusWidget()
{
    setupUi(this);

    auto palette = QGuiApplication::palette();

    masters_->setForegroundRole(QPalette::WindowText);
    activeMasters_->setForegroundRole(QPalette::WindowText);

    configs_->setForegroundRole(QPalette::WindowText);
    activeConfigs_->setForegroundRole(QPalette::WindowText);

    runners_->setForegroundRole(QPalette::WindowText);
    activeRunners_->setForegroundRole(QPalette::WindowText);

    streams_->setForegroundRole(QPalette::WindowText);
    activeStreams_->setForegroundRole(QPalette::WindowText);

    pending_->setForegroundRole(QPalette::WindowText);
    pendingLabel_->setForegroundRole(QPalette::WindowText);

    failures_->setForegroundRole(QPalette::WindowText);
    failuresLabel_->setForegroundRole(QPalette::WindowText);

    now_->setForegroundRole(QPalette::WindowText);
    elapsed_->setForegroundRole(QPalette::WindowText);


    QString zero(QString::number(0));
    activeMasters_->setText(zero);
    activeConfigs_->setText(zero);
    activeRunners_->setText(zero);
    activeStreams_->setText(zero);
    failures_->setText(zero);
}
