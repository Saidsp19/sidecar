#ifndef SIDECAR_GUI_HEALTHANDSTATUS_SAMPLEPLOTWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_HEALTHANDSTATUS_SAMPLEPLOTWIDGET_H

#include "QtGui/QWidget"

#include "Messages/Video.h"

#include "PlotDataPointDeque.h"

namespace Logger { class Log; }
namespace Utils { class RunningMedian; }

namespace SideCar {
namespace GUI {
namespace HealthAndStatus {

/** QWidget variant that plots time-series data.
 */
class SamplePlotWidget : public QWidget
{
    Q_OBJECT
    using Super = QWidget;
public:

    static Logger::Log& Log();

    /** Constructor.

        \param parent 
    */
    SamplePlotWidget(QWidget* parent = 0);

    /** Destructor.
     */
    ~SamplePlotWidget();

    void setExpectedVariance(double value);

    void setSamplesColor(const QColor& color);

    void clear();

    void addSample(double value);

    double getMinimumValue() const;

    double getMaximumValue() const;

    double getSampleValue() const { return sample_; }

    double getAverageSampleValue() const;

    void setExpected(double value);

    double getExpected() const { return expected_; }

private:

    void resizeEvent(QResizeEvent* event);

    void showEvent(QShowEvent* event);

    void paintEvent(QPaintEvent* event);

    void resizePlot();

    Utils::RunningMedian* plotStats_;
    PlotDataPointDeque plotDataPoints_;
    std::vector<QPoint> samplePoints_;
    double sample_;
    double expectedVariance_;
    double expected_;
    QColor samplesColor_;
};

} // end namespace HealthAndStatus
} // end namespace GUI
} // end namespace SideCar

#endif
