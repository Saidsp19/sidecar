#ifndef SIDECAR_GUI_SPECTRUM_SPECTRUMWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_SPECTRUMWIDGET_H

#include <cmath>

#include "QtCore/QBasicTimer"
#include "QtGui/QMatrix"
#include "QtGui/QPixmap"
#include "QtWidgets/QWidget"

#include "GUI/Color.h"
#include "Messages/PRIMessage.h"

#include "DoubleVector.h"
#include "ViewSettings.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Spectrum {

class AzimuthLatch;
class FFTSettings;
class Settings;
class ViewChanger;
class WorkRequest;

class SpectrumWidget : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    static Logger::Log& Log();

    /** Constructor. Creates a new Visualizer from scratch.

        \param parent widget parent
    */
    SpectrumWidget(QWidget* parent);

    void setData(const Messages::PRIMessage::Ref&, const fftw_complex* data);

    void setBackground(const QImage& iamge);

    void setFrozen(bool state);

    void setAzimuthLatch(AzimuthLatch* trigger);

    void saveToSettings(QSettings& settings);

    void restoreFromSettings(QSettings& settings);

    const ViewSettings& getFullView() const { return viewStack_.front(); }

    const ViewSettings& getCurrentView() const { return viewStack_.back(); }

    void dupView();

    bool canPopView() const { return viewStack_.size() > 1; }

    void setCurrentView(const ViewSettings& viewSettings);

    const QVector<QPointF>& getBins() const { return bins_; }

    double getPowerFromMagnitude(const fftw_complex& c) const
    {
        return 20.0 * ::log10(::sqrt(c[0] * c[0] + c[1] * c[1]) + 1.0) - dbOffset_;
    }

signals:

    void binsUpdated(const QVector<QPointF>& bins);

    void transformChanged();

    void currentCursorPosition(const QPointF& pos);

    void showMessageInfo(const Messages::PRIMessage::Ref& msg);

public slots:

    void setFFTSize(int size);

    void centerAtCursor();

    void needUpdate();

    void swapViews();

    void popView();

    void popAllViews();

    void zoom(const QPoint& from, const QPoint& to);

    void pan(const QPoint& from, const QPoint& to);

private slots:

    void updateColor(const QColor& color);

    void powerScalingChanged();

    void recalculateX();

private:
    void paintEvent(QPaintEvent* event);
    void resizeEvent(QResizeEvent* event);
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    void timerEvent(QTimerEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);

    void updateTransform();

    Settings* settings_;
    FFTSettings* fftSettings_;
    QBasicTimer updateTimer_;
    QPixmap background_;
    AzimuthLatch* azimuthLatch_;
    Messages::PRIMessage::Ref last_;
    QVector<QPointF> bins_;
    QVector<QPointF> points_;
    QMatrix transform_;
    QMatrix inverseTransform_;
    double sampleMin_;
    double dbOffset_;
    std::vector<ViewSettings> viewStack_;
    QPoint mouse_;
    QColor color_;

    ViewChanger* viewChanger_;

    bool needUpdate_;
    bool frozen_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
