#include "GUI/CLUTSetting.h"

#include "VideoImaging.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::BScope;

VideoImaging::VideoImaging(BoolSetting* visible, ColorButtonSetting* color, DoubleSetting* pointSize,
                           OpacitySetting* alpha, QComboBoxSetting* decimation, BoolSetting* colorMapEnabled,
                           CLUTSetting* clutSetting) :
    Super(visible, color, pointSize, alpha, decimation),
    colorMapEnabled_(colorMapEnabled), clutSetting_(clutSetting), colorMap_(),
    clut_(CLUT::Type(clutSetting->getValue()))
{
    connect(colorMapEnabled, SIGNAL(valueChanged(bool)), this, SLOT(setColorMapEnabled(bool)));
    connect(colorMapEnabled, SIGNAL(valueChanged(bool)), clutSetting, SLOT(setEnabled(bool)));
    connect(clutSetting, SIGNAL(valueChanged(int)), this, SLOT(colorMapTypeChanged(int)));
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
VideoImaging::updateColorMapImage()
{
    static const int kWidth = 256;
    static const int kHeight = 10;

    if (colorMap_.isNull()) colorMap_ = QImage(kWidth, kHeight, QImage::Format_RGB32);

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
