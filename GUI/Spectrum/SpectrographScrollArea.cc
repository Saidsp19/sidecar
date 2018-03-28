#include "QtGui/QScrollBar"

#include "GUI/LogUtils.h"

#include "App.h"
#include "Configuration.h"
#include "FFTSettings.h"
#include "FreqScaleWidget.h"
#include "ScaleWidget.h"
#include "SpectrographImaging.h"
#include "SpectrographScrollArea.h"

using namespace SideCar::GUI::Spectrum;

Logger::Log&
SpectrographScrollArea::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("spectrum.SpectrographScrollArea");
    return log_;
}

SpectrographScrollArea::SpectrographScrollArea(QWidget* parent) : Super(parent), firstShow_(true)
{
    App* app = App::GetApp();
    Configuration* configuration = app->getConfiguration();

    timeScale_ = new ScaleWidget(this, Qt::Vertical);
    int timeScaleWidth = timeScale_->minimumSizeHint().width();
    timeScale_->move(0, 2);
    timeScale_->resize(timeScale_->minimumSizeHint());

    SpectrographImaging* imaging = configuration->getSpectrographImaging();
    connect(imaging, SIGNAL(historySizeChanged(int)), SLOT(historySizeChanged(int)));

    timeScale_->setStart(imaging->getHistorySize());
    timeScale_->setEnd(0);
    timeScale_->setMajorTickDivisions(4);
    timeScale_->setLabel("FFTs");

    verticalScrollBar()->setTracking(true);
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(verticalPositionChanged(int)));

    freqScale_ = new FreqScaleWidget(this, Qt::Horizontal);
    int freqScaleHeight = freqScale_->minimumSizeHint().height();
    freqScale_->resize(freqScale_->minimumSizeHint());
    freqScale_->setMajorTickDivisions(4);
    freqScale_->setLabel("Frequency");

    FFTSettings* fftSettings = configuration->getFFTSettings();
    connect(fftSettings, SIGNAL(frequencyStepChanged(double)), SLOT(frequencyStepChanged(double)));
    setFrequencyScaleEnds(fftSettings->getFrequencyMin());

    setViewportMargins(timeScaleWidth, 0, 0, freqScaleHeight);
}

void
SpectrographScrollArea::resizeEvent(QResizeEvent* event)
{
    Logger::ProcLog log("resizeEvent", Log());
    LOGINFO << "size: " << size() << " viewport: " << viewport()->size() << ' ' << viewport()->pos() << std::endl;
    Super::resizeEvent(event);

    timeScale_->move(0, viewport()->pos().y());
    timeScale_->setSpan(viewport()->height());
    freqScale_->move(viewport()->pos().x(), height() - freqScale_->height() - 1);
    freqScale_->setSpan(viewport()->width());

    int vmax = verticalScrollBar()->maximum();
    LOGDEBUG << "vmax: " << vmax << std::endl;
    verticalScrollBar()->setValue(vmax);

    verticalPositionChanged(verticalScrollBar()->value());
}

void
SpectrographScrollArea::verticalPositionChanged(int value)
{
    static Logger::ProcLog log("verticalPositionChanged", Log());
    LOGERROR << value << std::endl;

    // At value == 0, we should show historySize --> historySize - height(). At value == maximum() we should
    // show height() --> 0
    //
    int range = widget()->height() - 1;
    int upper = range - value;
    int lower = upper - viewport()->height() + 1;
    LOGERROR << "range: " << range << " upper: " << upper << " lower: " << lower << std::endl;
    timeScale_->setStart(upper);
    timeScale_->setEnd(lower);
}

void
SpectrographScrollArea::frequencyStepChanged(double value)
{
    FFTSettings* fftSettings = qobject_cast<FFTSettings*>(sender());
    setFrequencyScaleEnds(fftSettings->getFrequencyMin());
}

void
SpectrographScrollArea::setFrequencyScaleEnds(double value)
{
    freqScale_->setStart(value);
    freqScale_->setEnd(-value);
}

void
SpectrographScrollArea::historySizeChanged(int historySize)
{
    Logger::ProcLog log("historySizeChanged", Log());
    LOGERROR << "historySizeChanged" << std::endl;
    timeScale_->setStart(historySize);
}

void
SpectrographScrollArea::currentCursorPositionChanged(const QPointF& pos)
{
    static Logger::ProcLog log("currentCursorPositionChanged", Log());
    LOGINFO << "pos: " << pos << std::endl;
    int y = int(pos.y()) - verticalScrollBar()->value();
    freqScale_->setCursorPosition(int(pos.x()));
    timeScale_->setCursorPosition(y);
    emit currentCursorPosition(freqScale_->formatTickTag(freqScale_->getValueAt(int(pos.x()))),
                               timeScale_->formatTickTag(timeScale_->getValueAt(y)));
}
