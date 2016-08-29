#include <cmath>

#include "App.h"
#include "CursorWidget.h"
#include "ViewSettings.h"
#include "Visualizer.h"

using namespace SideCar::GUI::AScope;

const int CursorWidget::kUpdateInterval = 250;

CursorWidget::CursorWidget(QWidget* parent)
    : Super(parent), app_(*App::GetApp()),
      updateTimer_(), x_(0.0), y_(0.0), isRange_(true),
      isVoltage_(true), updateX_(true), updateY_(true)
{
    connect(App::GetApp(), SIGNAL(distanceUnitsChanged(const QString&)),
            SLOT(refreshRange()));
    refresh();
}

void
CursorWidget::refreshRange()
{
    updateX_ = true;
    refresh();
}

void
CursorWidget::showCursorPosition(const QPoint& local, const QPointF& world)
{
    Visualizer* visualizer = qobject_cast<Visualizer*>(sender());
    const ViewSettings& view(visualizer->getCurrentView());

    bool doUpdate = false;

    if (x_ != world.x()) {
	x_ = world.x();
	isRange_ = view.isShowingRanges();
	doUpdate = true;
	updateX_ = true;
    }

    if (y_ != world.y()) {
	y_ = world.y();
	isVoltage_ = view.isShowingVoltages();
	doUpdate = true;
	updateY_ = true;
    }

    if (doUpdate) {
	if (! updateTimer_.isActive()) {
	    updateTimer_.start(kUpdateInterval, this);
	    refresh();
	}
    }
}

void
CursorWidget::refresh()
{
    if (updateX_) {
	updateX_ = false;
	if (isRange_)
	    xText_ = QString("Range: %1").arg(
		app_.getFormattedDistance(x_));
	else
	    xText_ = QString("Sample: %1").arg(int(::rint(x_)));
    }

    if (updateY_) {
	updateY_ = false;
	if (isVoltage_)
	    yText_ = QString(" Voltage: %1").arg(y_);
	else
	    yText_ = QString(" Count: %1").arg(int(::rint(y_)));
    }

    setValue(xText_ + yText_);
}

void
CursorWidget::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == updateTimer_.timerId()) {
	if (updateX_ || updateY_) {
	    refresh();
	}
	else {
	    updateTimer_.stop();
	}
    }
    else {
	Super::timerEvent(event);
    }
}
