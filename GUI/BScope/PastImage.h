#ifndef SIDECAR_GUI_BSCOPE_PASTIMAGE_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_PASTIMAGE_H

#include "QtCore/QBasicTimer"
#include "QtGui/QImage"
#include "QtGui/QPixmap"
#include "QtGui/QWidget"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class PhantomCursorImaging;
class ViewSettings;

namespace BScope {

class PastImage : public QWidget
{
    Q_OBJECT
    using Super = QWidget;
public:

    /** Obtain the Log device for PastImage instances

        \return Log device
    */
    static Logger::Log& Log();

    PastImage(QWidget* parent, const QImage& image,
              const QSize& size = QSize());

    void setImage(const QImage& image);

    void setImageSize(const QSize& size);

    const QImage& getImage() const { return image_; }

    QSize sizeHint() const { return imageSize_; }

    QSize minimumSizeHint() const { return imageSize_; }

    QSize maximumSizeHint() const { return imageSize_; }

    bool getShowPhantomCursor() const { return showPhantomCursor_; }

public slots:

    void setPhantomCursor(const QPointF& pos);

    void showPhantomCursor(bool state);

    void clearPhantomCursor();

    void setLabel(const QString& label);

private:
    
    void paintEvent(QPaintEvent* event);

    void checkCursorPosition();

    void enterEvent(QEvent* event);
    
    void leaveEvent(QEvent* event);
    
    void closeEvent(QCloseEvent* event);

    void timerEvent(QTimerEvent* event);

    QImage image_;
    QSize imageSize_;
    ViewSettings* viewSettings_;
    PhantomCursorImaging* phantomCursorImaging_;
    QPointF phantomCursor_;
    QBasicTimer updateTimer_;
    QPointF lastCursorPosition_;
    double azimuthMin_;
    double azimuthMax_;
    double rangeMin_;
    double rangeMax_;
    QString label_;
    bool showPhantomCursor_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
