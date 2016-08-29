#include "GUI/LogUtils.h"
#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "CursorWidget.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::BScope;

const int CursorWidget::kUpdateInterval = 250;

QString
CursorWidget::GetFormattedPosition(double azimuth, double range)
{
    return QString("Az: %1 Range: %2")
	.arg(App::GetApp()->getFormattedAngleRadians(
                 Utils::normalizeRadians(azimuth)))
	.arg(App::GetApp()->getFormattedDistance(range)) ;
}

CursorWidget::CursorWidget(QWidget* parent)
    : Super(parent), app_(App::GetApp()),
      viewSettings_(app_->getConfiguration()->getViewSettings()),
      azimuth_(0.0), range_(0.0), updateTimer_(), updateAzimuth_(true),
      updateRange_(true)
{
    connect(App::GetApp(), SIGNAL(distanceUnitsChanged(const QString&)),
            SLOT(refreshRange()));
    connect(App::GetApp(), SIGNAL(angleFormattingChanged(int)),
            SLOT(refreshAzimuth()));
    connect(App::GetApp(), SIGNAL(phantomCursorChanged(const QPointF&)),
            SLOT(showCursorPosition(const QPointF&)));
    refresh();
}

void
CursorWidget::refreshRange()
{
    updateRange_ = true;
    refresh();
}

void
CursorWidget::refreshAzimuth()
{
    updateAzimuth_ = true;
    refresh();
}

void
CursorWidget::showCursorPosition(const QPointF& pos)
{
    bool doUpdate = false;

    if (pos.x() == -1)
	return;

    // Incoming azimuth value lies between 0 and 1. Reconstitute into an actual azimuth value.
    //
    double azimuth = viewSettings_->azimuthFromParametric(pos.x());
    double range = viewSettings_->rangeFromParametric(pos.y());

    if (azimuth_ != azimuth) {
	azimuth_ = azimuth;
	doUpdate = true;
	updateAzimuth_ = true;
    }

    if (range_ != range) {
	range_ = range;
	doUpdate = true;
	updateRange_ = true;
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
    if (updateAzimuth_) {
	updateAzimuth_ = false;
	azimuthText_ = QString("Az: ") +
	    app_->getFormattedAngleRadians(
		Utils::normalizeRadians(azimuth_));
    }

    if (updateRange_) {
	updateRange_ = false;
	rangeText_ = QString(" Range: ") +
	    app_->getFormattedDistance(range_);
    }

    setValue(azimuthText_ + rangeText_);
    updateGeometry();
}

void
CursorWidget::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == updateTimer_.timerId()) {
	if (updateAzimuth_ || updateRange_) {
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
