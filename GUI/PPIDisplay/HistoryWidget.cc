#include "QtWidgets/QBoxLayout"
#include "QtWidgets/QCheckBox"
#include "QtWidgets/QSlider"

#include "App.h"
#include "Configuration.h"
#include "History.h"
#include "HistorySettings.h"
#include "HistoryWidget.h"

using namespace SideCar::GUI::PPIDisplay;

HistoryWidget::HistoryWidget(History* history, QToolBar* parent) : Super("Past", parent), history_(history)
{
    HistorySettings* historySettings = App::GetApp()->getConfiguration()->getHistorySettings();

    QCheckBox* enabled = new QCheckBox(this);
    historySettings->connectWidget(enabled);

    QBoxLayout* tmp = qobject_cast<QBoxLayout*>(layout());
    tmp->insertWidget(0, enabled, 0, Qt::AlignCenter);

    QSlider* slider = getControl();
    slider->setMinimum(0);
    slider->setMaximum(0);
    slider->setPageStep(1);

    connect(history_, SIGNAL(retainedCountChanged(int)), SLOT(historyRetainedCountChanged(int)));
    connect(history_, SIGNAL(currentViewChanged(int)), SLOT(historyCurrentViewChanged(int)));
    connect(history_, SIGNAL(currentViewAged(int)), SLOT(historyCurrentViewAged(int)));
    connect(slider, SIGNAL(valueChanged(int)), history, SLOT(showEntry(int)));

    slider->setEnabled(historySettings->isEnabled());

    connect(historySettings, SIGNAL(enabledChanged(bool)), slider, SLOT(setEnabled(bool)));
}

void
HistoryWidget::historyRetainedCountChanged(int size)
{
    QSlider* slider = getControl();
    slider->setEnabled(size);
    slider->setMaximum(size);
}

void
HistoryWidget::historyCurrentViewAged(int age)
{
    QSlider* slider = getControl();
    if (age <= history_->getRetentionCount()) {
        slider->blockSignals(true);
        slider->setValue(age);
        slider->blockSignals(false);
    }
    slider->setToolTip(makeAgeText(age));
}

void
HistoryWidget::historyCurrentViewChanged(int age)
{
    QSlider* slider = getControl();
    slider->blockSignals(true);
    slider->setValue(age);
    slider->blockSignals(false);
    slider->setToolTip(makeAgeText(age));
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
