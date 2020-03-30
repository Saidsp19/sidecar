#include "QtWidgets/QBoxLayout"
#include "QtWidgets/QCheckBox"
#include "QtWidgets/QLabel"
#include "QtWidgets/QSlider"
#include "QtWidgets/QToolBar"

#include "App.h"
#include "Configuration.h"
#include "History.h"
#include "HistorySettings.h"
#include "HistoryWidget.h"

using namespace SideCar::GUI::PPIDisplay;

static const int kMinLength = 100;
static const int kMaxLength = 200;

HistoryWidget::HistoryWidget(History* history, QToolBar* parent) : Super(parent), history_(history)
{
    HistorySettings* historySettings = App::GetApp()->getConfiguration()->getHistorySettings();
    auto orientation = parent->orientation();

    layout_ = new QBoxLayout(orientation == Qt::Vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight, this);
    layout_->setMargin(0);
    layout_->setSpacing(4);

    enabled_ = new QCheckBox("History", this);
    enabled_->setForegroundRole(QPalette::WindowText);
    historySettings->connectWidget(enabled_);
    layout_->addWidget(enabled_, 0, Qt::AlignCenter);

    layout_->addSpacing(8);

    slider_ = new QSlider(orientation, this);
    slider_->setMinimum(0);
    slider_->setMaximum(1);
    slider_->setValue(0);
    slider_->setPageStep(1);
    slider_->setEnabled(false);

    layout_->addWidget(slider_, 0, orientation == Qt::Vertical ? Qt::AlignHCenter : Qt::AlignVCenter);

    connect(history_, SIGNAL(retainedCountChanged(int)), SLOT(historyRetainedCountChanged(int)));
    connect(history_, SIGNAL(currentViewChanged(int)), SLOT(historyCurrentViewChanged(int)));
    connect(history_, SIGNAL(currentViewAged(int)), SLOT(historyCurrentViewAged(int)));

    connect(slider_, SIGNAL(valueChanged(int)), history, SLOT(showEntry(int)));

    connect(historySettings, SIGNAL(enabledChanged(bool)), SLOT(historyEnabledChanged(bool)));
    connect(parent, SIGNAL(orientationChanged(Qt::Orientation)), SLOT(changeOrientation(Qt::Orientation)));
}

void
HistoryWidget::updateOrientation(Qt::Orientation orientation)
{
    slider_->setOrientation(orientation);
    if (orientation == Qt::Vertical) {
        slider_->setMinimumSize(QSize(0, kMinLength));
        slider_->setMaximumSize(QSize(QWIDGETSIZE_MAX, kMaxLength));
        QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        policy.setHorizontalStretch(0);
        policy.setVerticalStretch(1);
        slider_->setSizePolicy(policy);
    } else {
        slider_->setMinimumSize(QSize(kMinLength, 0));
        slider_->setMaximumSize(QSize(kMaxLength, QWIDGETSIZE_MAX));
        QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        policy.setHorizontalStretch(1);
        policy.setVerticalStretch(0);
        slider_->setSizePolicy(policy);
    }
}

void
HistoryWidget::changeOrientation(Qt::Orientation orientation)
{
    updateOrientation(orientation);
    if (orientation == Qt::Horizontal) {
        layout_->setDirection(QBoxLayout::LeftToRight);
        layout_->setAlignment(slider_, Qt::AlignVCenter);
    } else {
        layout_->setDirection(QBoxLayout::TopToBottom);
        layout_->setAlignment(slider_, Qt::AlignHCenter);
    }
}

void
HistoryWidget::historyEnabledChanged(bool enabled)
{
    if (enabled) {
        historyRetainedCountChanged(history_->getRetentionCount());
    }
    else {
        slider_->setEnabled(false);
    }
}

void
HistoryWidget::historyRetainedCountChanged(int size)
{
    slider_->setEnabled(size > 0);
    slider_->setMaximum(std::max(size, 1));
}

void
HistoryWidget::historyCurrentViewAged(int age)
{
    if (age <= history_->getRetentionCount()) {
        slider_->blockSignals(true);
        slider_->setValue(age);
        slider_->blockSignals(false);
    }
    slider_->setToolTip(makeAgeText(age));
}

void
HistoryWidget::historyCurrentViewChanged(int age)
{
    slider_->blockSignals(true);
    slider_->setValue(age);
    slider_->blockSignals(false);
    slider_->setToolTip(makeAgeText(age));
}

QString
HistoryWidget::makeAgeText(int age)
{
    if (age == 0) {
        return "Now";
    } else {
        return QString("Now-%1").arg(age);
    }
}
