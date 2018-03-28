#include "QtCore/QSettings"

#include "App.h"
#include "Configuration.h"
#include "Settings.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::Spectrum;

static const char* const kRect = "Bounds";
static const char* const kXMin = "XMin";
static const char* const kXMax = "XMax";
static const char* const kYMin = "YMin";
static const char* const kYMax = "YMax";

int const ViewSettings::kMetaTypeId = qRegisterMetaType<ViewSettings>("ViewSettings");

ViewSettings::ViewSettings(double xMin, double xMax, double yMin, double yMax) :
    bounds_(), xMin_(xMin), xMax_(xMax), yMin_(yMin), yMax_(yMax)
{
    updateBounds();
}

void
ViewSettings::updateBounds()
{
    bounds_ = QRectF(xMin_, yMin_, xMax_ - xMin_, yMax_ - yMin_);
}

ViewSettings::ViewSettings(QSettings& settings)
{
    restoreFromSettings(settings);
}

void
ViewSettings::restoreFromSettings(QSettings& settings)
{
    bounds_ = settings.value(kRect).toRectF();
    xMin_ = settings.value(kXMin, xMin_).toDouble();
    xMax_ = settings.value(kXMax, xMax_).toDouble();
    yMin_ = settings.value(kYMin, yMin_).toDouble();
    yMax_ = settings.value(kYMax, yMax_).toDouble();
}

void
ViewSettings::saveToSettings(QSettings& settings) const
{
    settings.setValue(kRect, bounds_);
    settings.setValue(kXMin, xMin_);
    settings.setValue(kXMax, xMax_);
    settings.setValue(kYMin, yMin_);
    settings.setValue(kYMax, yMax_);
}

void
ViewSettings::setBounds(const QRectF& bounds)
{
    bounds_ = bounds;
    xMin_ = bounds.left();
    xMax_ = bounds.right();
    yMax_ = bounds.bottom();
    yMin_ = bounds.top();
}

bool
ViewSettings::operator==(const ViewSettings& rhs) const
{
    return bounds_ == rhs.bounds_;
}
