#include <cmath> // for ::rint

#include "GUI/Utils.h"
#include "Utils/Utils.h"

#include "PowerScaleWidget.h"

using namespace SideCar::GUI::Spectrum;

QString
PowerScaleWidget::formatTickTag(double value)
{
    return QString("%1").arg(value, 0, 'f', 1);
}
