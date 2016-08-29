#include "GUI/CLUTSetting.h"

#include "VideoImaging.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::PPIDisplay;

VideoImaging::VideoImaging(BoolSetting* visible,
                           ColorButtonSetting* color,
                           DoubleSetting* pointSize,
                           OpacitySetting* alpha,
                           QComboBoxSetting* decimation,
                           BoolSetting* colorMapEnabled,
                           CLUTSetting* clutSetting,
                           BoolSetting* desaturateEnabled,
                           IntSetting* desaturateRate)
    : Super(visible, color, pointSize, alpha, decimation),
      colorMapEnabled_(colorMapEnabled), clutSetting_(clutSetting),
      desaturateEnabled_(desaturateEnabled),
      desaturateRate_(desaturateRate), colorMap_(),
      clut_(CLUT::Type(clutSetting->getValue()))
{
    connect(colorMapEnabled, SIGNAL(valueChanged(bool)), this,
            SLOT(setColorMapEnabled(bool)));
    connect(colorMapEnabled, SIGNAL(valueChanged(bool)), clutSetting,
            SLOT(setEnabled(bool)));
    connect(clutSetting, SIGNAL(valueChanged(int)), this,
            SLOT(colorMapTypeChanged(int)));
    connect(desaturateEnabled, SIGNAL(valueChanged(bool)), this,
            SLOT(setDesaturateEnabled(bool)));
    connect(desaturateRate, SIGNAL(valueChanged(int)), this,
            SLOT(setDesaturateRate(int)));
    updateColorMapImage();
}

void
VideoImaging::setColorMapEnabled(bool value)
{
    updateColorMapImage();
    postSettingChanged();
}

void
VideoImaging::setColorMapType(int index)
{
    clutSetting_->setValue(index);
}

void
VideoImaging::colorMapTypeChanged(int value)
{
    clut_.setType(CLUT::Type(value));
    updateColorMapImage();
    postSettingChanged();
}

void
VideoImaging::setDesaturateEnabled(bool value)
{
    emit desaturationEnabledChanged();
}

void
VideoImaging::setDesaturateRate(int value)
{
    emit desaturationChanged();
}

void
VideoImaging::updateColorMapImage()
{
    if (colorMap_.isNull())
	colorMap_ = QImage(301, 10, QImage::Format_RGB32);

    if (getColorMapEnabled()) {
	clut_.makeColorMapImage(colorMap_, true);
    }
    else {
	clut_.makeGradiantImage(colorMap_, Super::getColor(), true);
    }

    emit colorMapChanged(colorMap_);
}

Color
VideoImaging::getColor(double intensity) const
{
    if (getColorMapEnabled()) {
	return clut_.getColor(intensity);
    }
    else {
	Color tmp(Super::getColor());
	return tmp *= intensity;
    }
}
