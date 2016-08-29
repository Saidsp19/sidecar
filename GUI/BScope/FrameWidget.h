#ifndef SIDECAR_GUI_BSCOPE_FRAMEWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_FRAMEWIDGET_H

#include "QtGui/QWidget"

class QImage;

namespace Logger { class Log; }
namespace SideCar {
namespace GUI {
namespace BScope {

class PastImage;
class ScaleWidget;

class FrameWidget : public QWidget
{
    Q_OBJECT
    using Super = QWidget;
public:

    static Logger::Log& Log();

    FrameWidget(QWidget* parent, const QImage& image,
                const QSize& size = QSize());

    QSize sizeHint() const { return size(); }

    PastImage* getPastImage() const { return frame_; }

    ScaleWidget* getAzimuthScale() const { return azimuthScale_; }

    ScaleWidget* getRangeScale() const { return rangeScale_; }

public slots:

    void setImage(const QImage& image);

    void setImageSize(const QSize& size);

    void setPhantomCursor(const QPointF& pos);

private slots:

    void updateScaleTicks();

    void updateScaleRanges();

private:

    void updateMinMaxSizes(const QSize& oldSize);

    PastImage* frame_;
    ScaleWidget* rangeScale_;
    ScaleWidget* azimuthScale_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
