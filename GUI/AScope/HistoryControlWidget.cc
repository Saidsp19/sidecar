#include "GUI/LogUtils.h"
#include "GUI/ToolBar.h"

#include "App.h"
#include "DisplayView.h"
#include "History.h"
#include "HistoryControlWidget.h"
#include "HistoryPosition.h"
#include "HistorySettings.h"
#include "Visualizer.h"

using namespace SideCar;
using namespace SideCar::GUI::AScope;

Logger::Log&
HistoryControlWidget::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("ascope.HistoryControlWidget");
    return log_;
}

HistoryControlWidget::HistoryControlWidget(ToolBar* parent)
    : Super(parent), Ui::HistoryControlWidget(),
      history_(App::GetApp()->getHistory()), activeHistoryPosition_(0)
{
    setupUi(this);

    setEnabled(false);
    pastSlider_->setEnabled(false);

    // Link our enabled state to that of the history setting.
    //
    HistorySettings& historySettings = App::GetApp()->getHistorySettings();
    historySettings.connectWidget(enabled_);

    enabled_->setChecked(historySettings.isEnabled());
    viewPast_->setEnabled(historySettings.isEnabled());
    synch_->setEnabled(historySettings.isEnabled());

    connect(&historySettings, SIGNAL(enabledChanged(bool)),
            SLOT(historyEnabledChanged(bool)));

    // Detect changes to the history mechanism. This keeps multiple HistoryControlWidget objects in synch.
    //
    connect(&history_, SIGNAL(pastFrozen()), SLOT(pastFrozen()));
    connect(&history_, SIGNAL(pastThawed()), SLOT(pastThawed()));
}

void
HistoryControlWidget::historyEnabledChanged(bool state)
{
    if (! state) {
	viewPast_->setChecked(false);
	pastSlider_->setEnabled(false);
    }

    viewPast_->setEnabled(state);
    synch_->setEnabled(state);
}
    
void
HistoryControlWidget::pastFrozen()
{
    pastSlider_->setEnabled(true);
    if (! viewPast_->isChecked())
	viewPast_->setChecked(true);
}

void
HistoryControlWidget::pastThawed()
{
    pastSlider_->setEnabled(false);
    if (viewPast_->isChecked())
	viewPast_->setChecked(false);
}

void
HistoryControlWidget::manage(HistoryPosition* historyPosition)
{
    connect(historyPosition, SIGNAL(viewingPastChanged(bool)),
            SLOT(viewingPastChanged(bool)));
    connect(viewPast_, SIGNAL(toggled(bool)), historyPosition,
            SLOT(viewingPast(bool)));
    connect(this, SIGNAL(positionChange(int)), historyPosition,
            SLOT(setPosition(int)));
}

void
HistoryControlWidget::viewingPastChanged(bool state)
{
    pastSlider_->setEnabled(state);
}

void
HistoryControlWidget::on_viewPast__toggled(bool state)
{
    pastSlider_->setEnabled(state);
    pastSlider_->setValue(0);
    if (state) {
	pastSlider_->setMaximum(history_.getFrameCount());
    }
}

void
HistoryControlWidget::on_pastSlider__valueChanged(int position)
{
    if (activeHistoryPosition_) {
	if (synch_->isChecked()) {
	    emit positionChange(position);
	}
	else {
	    activeHistoryPosition_->setPosition(position);
	}
    }
}

void
HistoryControlWidget::on_synch__toggled(bool state)
{
    if (activeHistoryPosition_)
	activeHistoryPosition_->setSynchronized(state);
}

void
HistoryControlWidget::activeDisplayViewChanged(DisplayView* displayView)
{
    if (! displayView) {
	activeHistoryPosition_ = 0;
	setEnabled(false);
    }
    else {
	setEnabled(true);
	activeHistoryPosition_ =
	    displayView->getVisualizer()->getHistoryPosition();
	synch_->setChecked(activeHistoryPosition_->isSynchronized());
	if (activeHistoryPosition_->isViewingPast()) {
	    pastSlider_->setEnabled(true);
	    pastSlider_->setValue(activeHistoryPosition_->getPastPosition());
	}
	else {
	    pastSlider_->setEnabled(false);
	}
    }
}
