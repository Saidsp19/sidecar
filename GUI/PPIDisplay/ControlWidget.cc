#include "QtWidgets/QBoxLayout"
#include "QtWidgets/QLabel"
#include "QtWidgets/QSlider"
#include "QtWidgets/QToolBar"

#include "ControlWidget.h"

using namespace SideCar::GUI::PPIDisplay;

static const int kMinLength = 100;
static const int kMaxLength = 200;

ControlWidget::ControlWidget(const QString& tag, QToolBar* parent) : Super(parent)
{
    Qt::Orientation orientation = parent->orientation();

    layout_ = new QBoxLayout(orientation == Qt::Vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight, this);
    layout_->setMargin(0);
    layout_->setSpacing(4);

    QLabel* label = new QLabel(tag, this);
    label->setAlignment(Qt::AlignCenter);
    QFont font(label->font());
    font.setPointSize(font.pointSize() - 1);
    label->setFont(font);
    label->setForegroundRole(QPalette::WindowText);
    layout_->addWidget(label, 0, Qt::AlignCenter);

    control_ = new QSlider(orientation, this);
    updateOrientation(orientation);

    layout_->addWidget(control_);
    layout_->setAlignment(control_, orientation == Qt::Vertical ? Qt::AlignHCenter : Qt::AlignVCenter);
    layout_->addSpacing(4);
    layout_->addStretch();

    connect(parent, SIGNAL(orientationChanged(Qt::Orientation)), SLOT(changeOrientation(Qt::Orientation)));
}

void
ControlWidget::updateOrientation(Qt::Orientation orientation)
{
    control_->setOrientation(orientation);
    if (orientation == Qt::Vertical) {
        control_->setMinimumSize(QSize(0, kMinLength));
        control_->setMaximumSize(QSize(QWIDGETSIZE_MAX, kMaxLength));
        QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        policy.setHorizontalStretch(0);
        policy.setVerticalStretch(1);
        control_->setSizePolicy(policy);
    } else {
        control_->setMinimumSize(QSize(kMinLength, 0));
        control_->setMaximumSize(QSize(kMaxLength, QWIDGETSIZE_MAX));
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        policy.setHorizontalStretch(1);
        policy.setVerticalStretch(0);
        control_->setSizePolicy(policy);
    }
}

void
ControlWidget::changeOrientation(Qt::Orientation orientation)
{
    updateOrientation(orientation);
    if (orientation == Qt::Horizontal) {
        layout_->setDirection(QBoxLayout::LeftToRight);
        layout_->setAlignment(control_, Qt::AlignVCenter);
    } else {
        layout_->setDirection(QBoxLayout::TopToBottom);
        layout_->setAlignment(control_, Qt::AlignHCenter);
    }
}
