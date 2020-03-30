#include "QtGui/QImage"
#include "QtGui/QPixmap"

#include "VTick.h"

using namespace SideCar::GUI;

QPixmap* VTick::tick_ = nullptr;

VTick::VTick(QWidget* parent) : QLabel(parent)
{
    if (tick_ ==  nullptr) {
        QImage image(1, 2, QImage::Format_ARGB32);
        for (int index = 0; index < 2; ++index) image.setPixel(0, index, QColor(127, 127, 127).rgba());
        tick_ = new QPixmap(QPixmap::fromImage(image));
    }

    setPixmap(*tick_);
}
