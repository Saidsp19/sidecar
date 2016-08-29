#include "ChannelImaging.h"

using namespace SideCar::GUI;

ChannelImaging::ChannelImaging(BoolSetting* enabled, ColorButtonSetting* color, DoubleSetting* size,
                               OpacitySetting* opacity)
    : Super(enabled), qcolor_(color), size_(size)
{
    connect(color, SIGNAL(valueChanged(const QColor&)), this, SLOT(colorChanged(const QColor&)));
    connect(size, SIGNAL(valueChanged(double)), this, SLOT(sizeChanged(double)));
    connect(opacity, SIGNAL(valueChanged(double)), this, SLOT(opacityChanged(double)));
    QColor c(color->getValue());
    color_ = Color(c.redF(), c.greenF(), c.blueF(), opacity->getValue());
}

void
ChannelImaging::colorChanged(const QColor& color)
{
    color_ = Color(color.redF(), color.greenF(), color.blueF(), color_.alpha);
    emit colorChanged();
    postSettingChanged();
}

void
ChannelImaging::sizeChanged(double size)
{
    emit sizeChanged();
    postSettingChanged();
}

void
ChannelImaging::opacityChanged(double value)
{
    color_.alpha = value;
    emit alphaChanged();
    postSettingChanged();
}
