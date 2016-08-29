#include "ImageScaler.h"

using namespace SideCar::GUI::BScope;

ImageScaler::ImageScaler(QObject* parent)
    : QObject(parent)
{
    ;
}

ImageScaler::~ImageScaler()
{
    ;
}

void
ImageScaler::scaleImage(const QImage& image, QSize newSize, int index)
{
    emit done(image.scaled(newSize, Qt::IgnoreAspectRatio,
                           Qt::SmoothTransformation), index);
}
