#include "Utils/Utils.h"

#include "App.h"
#include "CursorPosition.h"
#include "History.h"

using namespace SideCar::GUI::BScope;

struct CursorPosition::Private {
    Private(double x, double y, double azimuth, double range)
	: x_(x), y_(y), azimuth_(Utils::normalizeRadians(azimuth)),
	  range_(range), sampleValue_("NA"), toolTip_(),
	  widgetText_() {}

    double x_;
    double y_;
    double azimuth_;
    double range_;
    QString sampleValue_;

    mutable QString toolTip_;
    mutable QString widgetText_;
};

CursorPosition::CursorPosition(double x, double y, double azimuth,
                               double range)
    : Super(0), p_(new Private(x, y, azimuth, range))
{
    ;
}

CursorPosition::~CursorPosition()
{
    ;
}

double
CursorPosition::getX() const { return p_->x_; }

double
CursorPosition::getY() const { return p_->y_; }

double
CursorPosition::getAzimuth() const { return p_->azimuth_; }

double
CursorPosition::getRange() const { return p_->range_; }

void
CursorPosition::setSampleValue(const QString& sampleValue)
{
    p_->sampleValue_ = sampleValue;
}

void
CursorPosition::clearCache()
{
    if (! p_->toolTip_.isNull())
	p_->toolTip_ = QString();
    if (! p_->widgetText_.isNull())
	p_->widgetText_ = QString();
}

const QString&
CursorPosition::getToolTip() const
{
    if (p_->toolTip_.isNull())
	p_->toolTip_ = QString("%1 %2 [%3]")
	    .arg(App::GetApp()->getFormattedAngleRadians(p_->azimuth_))
	    .arg(App::GetApp()->getFormattedDistance(p_->range_))
	    .arg(p_->sampleValue_);
    return p_->toolTip_;
}

const QString&
CursorPosition::getWidgetText() const
{
    if (p_->widgetText_.isNull())
	p_->widgetText_ = QString("Az: %1 Range: %2 [%3]")
	    .arg(App::GetApp()->getFormattedAngleRadians(p_->azimuth_))
	    .arg(App::GetApp()->getFormattedDistance(p_->range_))
	    .arg(p_->sampleValue_);
    return p_->widgetText_;
}
