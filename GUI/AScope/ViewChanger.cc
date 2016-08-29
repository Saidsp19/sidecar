#include "ViewChanger.h"
#include "Visualizer.h"

using namespace SideCar::GUI::AScope;

ViewChanger::ViewChanger(Visualizer* visualizer, const QPoint& start)
    : visualizer_(visualizer), start_(start)
{
    ;
}

PanningViewChanger::PanningViewChanger(Visualizer* visualizer,
                                       const QPoint& start)
    : ViewChanger(visualizer, start)
{
    visualizer->dupView();
}

PanningViewChanger::~PanningViewChanger()
{
    if (visualizer_) visualizer_->popView();
}

void
PanningViewChanger::mouseMoved(const QPoint& pos)
{
    visualizer_->pan(start_, pos);
    start_ = pos;
}

void
PanningViewChanger::finished(const QPoint& pos)
{
    visualizer_  = 0;
}

ZoomingViewChanger::ZoomingViewChanger(Visualizer* visualizer,
                                       const QPoint& start)
    : ViewChanger(visualizer, start),
      rubberBand_(QRubberBand::Rectangle, visualizer)
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
	visualizer_->dupView();
	visualizer_->zoom(start_, pos);
    }
}
