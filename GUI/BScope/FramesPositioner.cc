#include "FramesPositioner.h"

using namespace SideCar::GUI::BScope;

FramesPositioner::FramesPositioner(QWidget* parent)
    : Super(parent), Ui::FramesPositioner()
{
    setupUi(this);
    position_->setValue(0);
    position_->setMaximum(0);
}

void
FramesPositioner::setPosition(int position)
{
    position_->setValue(position);
}

void
FramesPositioner::setMaximum(int maximum)
{
    position_->setMaximum(maximum);
}

void
FramesPositioner::updateInfo()
{
    int index = position_->value() + 1;
    info_->setText(QString("T-%1").arg(index));
}

void
FramesPositioner::on_position__valueChanged(int value)
{
    updateInfo();
    emit positionChanged(value);
}
