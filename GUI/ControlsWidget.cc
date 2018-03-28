#include "QtGui/QBoxLayout"
#include "QtGui/QLabel"
#include "QtGui/QSlider"
#include "QtGui/QToolBar"

#include "ControlsWidget.h"
#include "QSliderSetting.h"

using namespace SideCar::GUI;

static const int kMinLength = 100;
static const int kMaxLength = 500;

ControlsWidget::ControlsWidget(QToolBar* parent) : Super(parent)
{
    Qt::Orientation orientation = getOrientation();

    layout_ = new QBoxLayout(orientation == Qt::Vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight, this);
    layout_->setMargin(0);
    layout_->setSpacing(6);

    connect(parent, SIGNAL(orientationChanged(Qt::Orientation)), SLOT(changeOrientation(Qt::Orientation)));

    layout_->addSpacing(4);
    layout_->addStretch();
}

Qt::Orientation
ControlsWidget::getOrientation() const
{
    return qobject_cast<QToolBar*>(parentWidget())->orientation();
}

void
ControlsWidget::addControl(const QString& tag, QSliderSetting* setting)
{
    Qt::Orientation orientation = getOrientation();

    int index = layout_->count() - 2;

    QLabel* label = new QLabel(tag, this);
    label->setAlignment(Qt::AlignCenter);
    QFont font(label->font());
    font.setPointSize(9);
    font.setBold(true);
    label->setFont(font);
    layout_->insertWidget(index++, label, 0, Qt::AlignCenter);

    QSlider* control = new QSlider(orientation, this);
    controls_.append(control);
    setting->connectWidget(control);

    updateOrientation(orientation, control);
    layout_->insertWidget(index++, control, 1, 0);

    layout_->setAlignment(control, orientation == Qt::Vertical ? Qt::AlignHCenter : Qt::AlignVCenter);
}

void
ControlsWidget::updateOrientation(Qt::Orientation orientation, QSlider* control)
{
    control->setOrientation(orientation);
    if (orientation == Qt::Vertical) {
        control->setMinimumSize(QSize(0, kMinLength));
        control->setMaximumSize(QSize(QWIDGETSIZE_MAX, kMaxLength));
        QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        policy.setHorizontalStretch(0);
        policy.setVerticalStretch(1);
        control->setSizePolicy(policy);
        layout_->setAlignment(control, Qt::AlignHCenter);
    } else {
        control->setMinimumSize(QSize(kMinLength, 0));
        control->setMaximumSize(QSize(kMaxLength, QWIDGETSIZE_MAX));
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        policy.setHorizontalStretch(1);
        policy.setVerticalStretch(0);
        control->setSizePolicy(policy);
        layout_->setAlignment(control, Qt::AlignVCenter);
    }
}

void
ControlsWidget::changeOrientation(Qt::Orientation orientation)
{
    for (int index = 0; index < controls_.size(); ++index) updateOrientation(orientation, controls_[index]);

    if (orientation == Qt::Horizontal) {
        layout_->setDirection(QBoxLayout::LeftToRight);
    } else {
        layout_->setDirection(QBoxLayout::TopToBottom);
    }
}
