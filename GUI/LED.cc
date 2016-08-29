#include "QtSvg/QSvgRenderer"

#include "LED.h"

using namespace SideCar::GUI;

static const QString colorNames_[LED::kTurquoise + 1] = {
    "grey",
    "red",
    "green",
    "blue",
    "yellow",
    "orange",
    "turquoise"
};

LED::LED(QWidget* parent)
    : Super(parent), color_(), diameter_(0)
{
    setDiameter(24);
    setColor(kGrey);
}

void
LED::setDiameter(int value)
{
    if (value != diameter_) {
	diameter_ = value;
	QSize size(diameter_, diameter_);
	setMinimumSize(size);
	setMaximumSize(size);
    }
}

void
LED::setColor(Color value)
{
    if (value < kGrey || value > kTurquoise)
	value = kGrey;

    if (value != color_ || ! renderer()->isValid()) {
	color_ = value;
	load(QString(":/LED/%1.svg").arg(colorNames_[value]));
	update();
    }
}

void
LED::cycleColors()
{
    setColor(Color((color_ + 1)));
}
