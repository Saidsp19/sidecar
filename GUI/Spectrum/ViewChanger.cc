#include "ViewChanger.h"
#include "SpectrumWidget.h"

using namespace SideCar::GUI::Spectrum;

ViewChanger::ViewChanger(SpectrumWidget* spectrumWidget, const QPoint& start)
    : spectrumWidget_(spectrumWidget), start_(start)
{
    ;
}

PanningViewChanger::PanningViewChanger(SpectrumWidget* spectrumWidget,
                                       const QPoint& start)
    : ViewChanger(spectrumWidget, start)
{
    spectrumWidget->dupView();
}

PanningViewChanger::~PanningViewChanger()
{
    if (spectrumWidget_) spectrumWidget_->popView();
}

void
PanningViewChanger::mouseMoved(const QPoint& pos)
{
    spectrumWidget_->pan(start_, pos);
    start_ = pos;
}

void
PanningViewChanger::finished(const QPoint& pos)
{
    spectrumWidget_  = 0;
}

ZoomingViewChanger::ZoomingViewChanger(SpectrumWidget* spectrumWidget,
                                       const QPoint& start)
    : ViewChanger(spectrumWidget, start),
      rubberBand_(QRubberBand::Rectangle, spectrumWidget)
{
    rubberBand_.setGeometry(QRect(start, QSize()));
    rubberBand_.show();
}

void
ZoomingViewChanger::mouseMoved(const QPoint& pos)
{
    rubberBand_.setGeometry(QRect(start_, pos).normalized());
}

void
ZoomingViewChanger::finished(const QPoint& pos)
{
    rubberBand_.hide();
    if (pos.x() != start_.x() && pos.y() != start_.y()) {
	spectrumWidget_->dupView();
	spectrumWidget_->zoom(start_, pos);
    }
}
