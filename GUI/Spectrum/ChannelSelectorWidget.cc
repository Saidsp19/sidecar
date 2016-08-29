#include "ChannelSelectorWidget.h"

using namespace SideCar::GUI::Spectrum;

ChannelSelectorWidget::ChannelSelectorWidget(QWidget* parent)
    : QWidget(parent), Ui::ChannelSelectorWidget()
{
    setupUi(this);
}
