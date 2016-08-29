#include <cmath>

#include "GUI/Utils.h"
#include "Utils/Utils.h"

#include "FreqScaleWidget.h"

using namespace SideCar::GUI::Spectrum;

QString
FreqScaleWidget::formatTickTag(double value)
{
    double tmp = std::abs(value);
    if (tmp >= 1.0E6)
	return QString("%1 MHz").arg(value / 1.0E6, 0, 'f', 1);

    if (tmp >= 1.0E3)
	return QString("%1 kHz").arg(value / 1.0E3, 0, 'f', 1);

    return QString("%1 Hz").arg(value, 0, 'f', 1);
}
