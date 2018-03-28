#include "QtGui/QBoxLayout"
#include "QtGui/QLabel"
#include "QtGui/QSlider"
#include "QtGui/QToolBar"

#include "GUI/QSliderSetting.h"

#include "App.h"
#include "Configuration.h"
#include "ControlsWidget.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::BScope;

static const int kMinLength = 100;
static const int kMaxLength = 200;

ControlsWidget::ControlsWidget(QToolBar* parent) : Super(parent)
{
    Qt::Orientation orientation = getOrientation();

    layout_ = new QBoxLayout(orientation == Qt::Vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight, this);
    layout_->setMargin(0);
    layout_->setSpacing(6);

    Configuration* configuration = App::GetApp()->getConfiguration();
    addControl("Gain", configuration->getGainSetting());
    addControl("Min", configuration->getCutoffMinSetting());
    addControl("Max", configuration->getCutoffMaxSetting());

    connect(parent, SIGNAL(orientationChanged(Qt::Orientation)), SLOT(changeOrientation(Qt::Orientation)));
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

    QLabel* label = new QLabel(tag, this);
    label->setAlignment(Qt::AlignCenter);
    QFont font(label->font());
    font.setPointSize(9);
    font.setBold(true);
    label->setFont(font);
    layout_->addWidget(label, 0, Qt::AlignCenter);

    QSlider* control = new QSlider(orientation, this);
    controls_.append(control);
    setting->connectWidget(control);

    updateOrientation(orientation, control);
    layout_->addWidget(control);
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
