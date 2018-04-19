#include "QtWidgets/QHBoxLayout"
#include "QtWidgets/QLabel"

#include "HistoryFrame.h"
#include "RadarInfoWidget.h"

using namespace SideCar::GUI::AScope;

RadarInfoWidget::RadarInfoWidget(QWidget* parent) :
    Super(parent), shaft_(0), shaftText_(" Shaft: 0"), updateShaft_(true)
{
    refresh();
}

void
RadarInfoWidget::showMessageInfo(const Messages::PRIMessage::Ref& msg)
{
    if (!msg) return;

    if (msg->getRIUInfo().shaftEncoding != shaft_) {
        shaft_ = msg->getRIUInfo().shaftEncoding;
        updateShaft_ = true;
    }

    Super::showMessageInfo(msg);
}

QString
RadarInfoWidget::makeLabel()
{
    if (updateShaft_) {
        updateShaft_ = false;
        shaftText_ = QString(" Shaft: ") + QString::number(shaft_);
    }

    return Super::makeLabel() + shaftText_;
}
