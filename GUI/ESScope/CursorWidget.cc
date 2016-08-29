#include "GUI/LogUtils.h"
#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "CursorWidget.h"
#include "RadarSettings.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::ESScope;

const int CursorWidget::kUpdateInterval = 250;

CursorWidget::CursorWidget(QWidget* parent)
    : Super(parent), app_(App::GetApp()),
      radarSettings_(app_->getConfiguration()->getRadarSettings()),
      alpha_(0.0), beta_(0.0), range_(0.0), updateTimer_(),
      updateAlphaBeta_(true), updateRange_(true)
{
    connect(App::GetApp(), SIGNAL(angleFormattingChanged(int)),
            SLOT(refreshAlphaBeta()));
    connect(App::GetApp(), SIGNAL(distanceUnitsChanged(const QString&)),
            SLOT(refreshRange()));
    connect(radarSettings_, SIGNAL(tiltChanged(double)),
            SLOT(refreshAlphaBeta()));
    connect(radarSettings_, SIGNAL(rotationChanged(double)),
            SLOT(refreshAlphaBeta()));
    refresh();
}

void
CursorWidget::setAlphaBeta(double alpha, double beta)
{
    bool doUpdate = false;

    if (alpha_ != alpha) {
	alpha_ = alpha;
	doUpdate = true;
	updateAlphaBeta_ = true;
    }

    if (beta_ != beta) {
	beta_ = beta;
	doUpdate = true;
	updateAlphaBeta_ = true;
    }

    if (doUpdate) {
	if (! updateTimer_.isActive()) {
	    updateTimer_.start(kUpdateInterval, this);
	    refresh();
	}
    }
}

void
CursorWidget::refreshAlphaBeta()
{
    updateAlphaBeta_ = true;
    refresh();
}

void
CursorWidget::setRange(double dummy, double range)
{
    bool doUpdate = false;

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
CursorWidget::refreshRange()
{
    updateRange_ = true;
    refresh();
}

void
CursorWidget::refresh()
{
    if (updateAlphaBeta_) {
	updateAlphaBeta_ = false;
	double azimuth, elevation;
	radarSettings_->getAzimuthElevation(alpha_, beta_, &azimuth,
                                            &elevation);
	azimuthElevationText_ = QString("Az: %1  El: %2")
	    .arg(app_->getFormattedAngleRadians(
                     Utils::normalizeRadians(azimuth)))
	    .arg(app_->getFormattedAngleRadians(
                     Utils::normalizeRadians(elevation)));
    }

    if (updateRange_) {
	updateRange_ = false;
	rangeText_ = QString("  R: ") +
	    app_->getFormattedDistance(range_);
    }

    setValue(azimuthElevationText_ + rangeText_);
    updateGeometry();
}

void
CursorWidget::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == updateTimer_.timerId()) {
	if (updateAlphaBeta_ || updateRange_) {
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
