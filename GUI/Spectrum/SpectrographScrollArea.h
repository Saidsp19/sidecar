#ifndef SIDECAR_GUI_SPECTRUM_SPECTROGRAPHSCROLLAREA_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_SPECTROGRAPHSCROLLAREA_H

#include "QtWidgets/QScrollArea"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Spectrum {

class FreqScaleWidget;
class ScaleWidget;

class SpectrographScrollArea : public QScrollArea {
    Q_OBJECT
    using Super = QScrollArea;

public:
    static Logger::Log& Log();

    SpectrographScrollArea(QWidget* parent);

signals:

    void currentCursorPosition(const QString& x, const QString& y);

public slots:

    void currentCursorPositionChanged(const QPointF& pos);

private slots:

    void verticalPositionChanged(int value);

    void frequencyStepChanged(double value);

    void historySizeChanged(int value);

private:
    void resizeEvent(QResizeEvent* event);

    void setFrequencyScaleEnds(double value);

    ScaleWidget* timeScale_;
    ScaleWidget* freqScale_;
    bool firstShow_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
