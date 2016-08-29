#include "QtGui/QImage"
#include "QtGui/QPixmap"

#include "VTick.h"

using namespace SideCar::GUI;

QPixmap* VTick::tick_ = 0;

VTick::VTick(QWidget* parent)
    : QLabel(parent)
{
    if (! tick_) {
	QImage image(1, 2, QImage::Format_ARGB32);
	for (int index = 0; index < 2; ++index)
	    image.setPixel(0, index, QColor(0, 0, 0).rgba());
	tick_ = new QPixmap(QPixmap::fromImage(image));
    }

    setPixmap(*tick_);
}
