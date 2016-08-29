#include "GUI/ColorButtonSetting.h"
#include "GUI/LogUtils.h"

#include "SpectrographImaging.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Spectrum;

static const int kColorMapHeight = 10;

Logger::Log&
SpectrographImaging::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("spectrum.SpectrographImaging");
    return log_;
}

SpectrographImaging::SpectrographImaging(BoolSetting* enabled,
                                         ColorButtonSetting* color,
                                         DoubleSetting* pointSize,
                                         OpacitySetting* opacity,
                                         BoolSetting* colorMapEnabled,
                                         QComboBoxSetting* colorMapType,
                                         QDoubleSpinBoxSetting* minCutoff,
                                         QDoubleSpinBoxSetting* maxCutoff,
                                         IntSetting* historySize)
: Super(enabled, color, pointSize, opacity),
  colorMapEnabled_(colorMapEnabled), colorMap_(),
  clut_(CLUT::kBlueSaturated), colorMapType_(colorMapType),
  minCutoff_(minCutoff), maxCutoff_(maxCutoff),
  historySize_(historySize)
{
    connect(colorMapEnabled, SIGNAL(valueChanged(bool)),
            SLOT(setColorMapEnabled(bool)));
    connect(colorMapType, SIGNAL(valueChanged(int)),
            SLOT(setColorMapType(int)));

    connect(minCutoff, SIGNAL(valueChanged(double)),
            SIGNAL(minCutoffChanged(double)));
    connect(maxCutoff, SIGNAL(valueChanged(double)),
            SIGNAL(maxCutoffChanged(double)));

    connect(minCutoff, SIGNAL(valueChanged(double)),
            SLOT(updateDbColorTransform()));
    connect(maxCutoff, SIGNAL(valueChanged(double)),
            SLOT(updateDbColorTransform()));

    connect(historySize, SIGNAL(valueChanged(int)),
            SIGNAL(historySizeChanged(int)));

    updateDbColorTransform();
    updateColorMapImage();
}

void
SpectrographImaging::setColorMapEnabled(bool value)
{
    colorMapType_->setEnabled(value);
    updateColorMapImage();
    postSettingChanged();
}

void
SpectrographImaging::setColorMapType(int value)
{
    Logger::ProcLog log("setColorMapType", Log());
    LOGINFO << "value: " << value << std::endl;
    clut_.setType(CLUT::Type(value));
    updateColorMapImage();
    postSettingChanged();
}

void
SpectrographImaging::updateColorMapImage()
{
    if (colorMap_.isNull())
	colorMap_ = QImage(256, 10, QImage::Format_RGB32);

    if (getColorMapEnabled()) {
	clut_.makeColorMapImage(colorMap_, true);
    }
    else {
	clut_.makeGradiantImage(colorMap_, Super::getColor(), true);
    }

    emit colorMapChanged(colorMap_);
}

void
SpectrographImaging::updateDbColorTransform()
{
    offset_ = minCutoff_->getValue();
    scaling_ = 1.0 / (maxCutoff_->getValue() - offset_);
}

Color
SpectrographImaging::getColor(double db) const
{
    static Logger::ProcLog log("getColor", Log());
    LOGINFO << "db: " << db << std::endl;

    double normalized = (db - offset_) * scaling_;
    if (normalized < 0.0) normalized = 0.0;
    else if (normalized > 1.0) normalized = 1.0;

    if (getColorMapEnabled()) {
	return clut_.getColor(normalized);
    }
    else {
	Color tmp(Super::getColor());
	return tmp *= normalized;
    }
}
