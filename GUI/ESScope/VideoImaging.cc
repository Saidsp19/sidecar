#include "GUI/CLUTSetting.h"

#include "VideoImaging.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ESScope;

VideoImaging::VideoImaging(BoolSetting* visible, ColorButtonSetting* color, DoubleSetting* pointSize,
                           OpacitySetting* alpha, QComboBoxSetting* decimation, BoolSetting* colorMapEnabled,
                           CLUTSetting* colorMapType) :
    Super(visible, color, pointSize, alpha, decimation),
    colorMapEnabled_(colorMapEnabled), colorMap_(), clut_(CLUT::Type(colorMapType->getValue()))
{
    connect(colorMapEnabled, SIGNAL(valueChanged(bool)), this, SLOT(setColorMapEnabled(bool)));
    connect(colorMapEnabled, SIGNAL(valueChanged(bool)), colorMapType, SLOT(setEnabled(bool)));
    connect(colorMapType, SIGNAL(valueChanged(int)), this, SLOT(setColorMapType(int)));
    updateColorMapImage();
}

void
VideoImaging::setColorMapEnabled(bool value)
{
    updateColorMapImage();
    postSettingChanged();
}

void
VideoImaging::setColorMapType(int value)
{
    clut_.setType(CLUT::Type(value));
    updateColorMapImage();
    postSettingChanged();
}

void
VideoImaging::updateColorMapImage()
{
    if (colorMap_.isNull()) colorMap_ = QImage(256, 10, QImage::Format_RGB32);

    if (getColorMapEnabled()) {
        clut_.makeColorMapImage(colorMap_, true);
    } else {
        clut_.makeGradiantImage(colorMap_, Color(0.0, 1.0, 0.0), true);
    }

    emit colorMapChanged(colorMap_);
}

Color
VideoImaging::getColor(double intensity) const
{
    if (getColorMapEnabled()) {
        return clut_.getColor(intensity);
    } else {
        Color tmp(Super::getColor());
        return tmp *= intensity;
    }
}
