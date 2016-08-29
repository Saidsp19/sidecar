#ifndef SIDECAR_GUI_BSCOPE_IMAGESCALER_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_IMAGESCALER_H

#include "QtCore/QObject"
#include "QtGui/QImage"

namespace SideCar {
namespace GUI {
namespace BScope {

class ImageScaler : public QObject
{
    Q_OBJECT
public:

    ImageScaler(QObject* parent = 0);

    ~ImageScaler();

signals:

    void done(QImage image, int index);

public slots:

    void scaleImage(const QImage& image, QSize newSize, int index);

private:
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
