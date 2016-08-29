#include "QtCore/QSettings"
#include "QtGui/QPainter"
#include "QtGui/QResizeEvent"
#include "QtGui/QGridLayout"

#include "GUI/LogUtils.h"

#include "DisplayView.h"
#include "ScaleWidget.h"
#include "Visualizer.h"

using namespace SideCar::GUI::AScope;

DisplayView* DisplayView::activeDisplayView_ = 0;

Logger::Log&
DisplayView::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.DisplayView");
    return log_;
}

DisplayView::DisplayView(QWidget* parent, AzimuthLatch* azimuthLatch)
    : QFrame(parent), visualizer_(new Visualizer(this, azimuthLatch)),
      horizontalScale_(new ScaleWidget(this, Qt::Horizontal)),
      verticalScale_(new ScaleWidget(this, Qt::Vertical))
{
    initialize();
}

DisplayView::~DisplayView()
{
    static Logger::ProcLog log("~DisplayView", Log());
    LOGINFO << this << " + " << visualizer_ << std::endl;
    if (activeDisplayView_ == this) {
	activeDisplayView_ = 0;
	emit activeDisplayViewChanged(0);
    }
}

void
DisplayView::saveToSettings(QSettings& settings)
{
    Logger::ProcLog log("saveToSettings", Log());
    LOGINFO << settings.group() << std::endl;
    settings.beginGroup("Visualizer");
    visualizer_->saveToSettings(settings);
    settings.endGroup();
    LOGINFO << "done" << std::endl;
}

void
DisplayView::restoreFromSettings(QSettings& settings)
{
    Logger::ProcLog log("restoreFromSettings", Log());
    LOGINFO << settings.group() << std::endl;
    settings.beginGroup("Visualizer");
    visualizer_->restoreFromSettings(settings);
    settings.endGroup();
    makeBackground();
    LOGINFO << "done" << std::endl;
}

void
DisplayView::duplicate(const DisplayView* other)
{
    visualizer_->duplicate(other->visualizer_);
}

void
DisplayView::initialize()
{
    static Logger::ProcLog log("initialize", Log());
    LOGINFO << std::endl;

    setFrameStyle(QFrame::Box | QFrame::Plain);
    setLineWidth(2);

    // NOTE: use Qt::QueuedConnection with the transformChanged() signal so that all widget resizing is done
    // before we do anything with the new transform. Specifically, we need our Scale widgets to adjust before we
    // use them to update the background pixmap intalled in the Visualizer object.
    //
    connect(visualizer_, SIGNAL(transformChanged()),
            SLOT(visualizerTransformChanged()),
            Qt::QueuedConnection);

    connect(visualizer_,
            SIGNAL(pointerMoved(const QPoint&, const QPointF&)),
            SLOT(updateCursorPosition(const QPoint&, const QPointF&)));

    const ViewBounds& viewRect(visualizer_->getCurrentView().getBounds());
    horizontalScale_->setStart(viewRect.getXMin());
    horizontalScale_->setRange(viewRect.getWidth());
    verticalScale_->setStart(viewRect.getYMin());
    verticalScale_->setRange(viewRect.getHeight());

    QGridLayout* layout = new QGridLayout;
    layout->setSpacing(0);
    layout->addWidget(verticalScale_, 0, 0);
    layout->addWidget(visualizer_, 0, 1);
    layout->addWidget(horizontalScale_, 1, 1);
    setLayout(layout);

    makeBackground();
}

void
DisplayView::visualizerTransformChanged()
{
    static Logger::ProcLog log("visualizerTransformChanged", Log());
    LOGINFO << std::endl;

    const ViewSettings& viewSettings(visualizer_->getCurrentView());
    const ViewBounds& viewRect(viewSettings.getBounds());
    horizontalScale_->setStart(viewRect.getXMin());
    horizontalScale_->setRange(viewRect.getWidth());
    verticalScale_->setStart(viewRect.getYMin());
    verticalScale_->setRange(viewRect.getHeight());
    makeBackground();
}

void
DisplayView::makeBackground()
{
    static Logger::ProcLog log("makeBackground", Log());
    LOGINFO << std::endl;

    QImage image(visualizer_->width(), visualizer_->height(),
                 QImage::Format_RGB32);
    if (! image.isNull()) {
	QPainter painter(&image);
	painter.setBackground(Qt::black);
	painter.eraseRect(image.rect());
	if (visualizer_->isShowingGrid()) {
	    horizontalScale_->drawGridLines(painter);
	    verticalScale_->drawGridLines(painter);
	}
	painter.end();
	visualizer_->setBackground(image);
    }
}

void
DisplayView::updateCursorPosition(const QPoint& local, const QPointF& world)
{
    horizontalScale_->setCursorPosition(local.x());
    verticalScale_->setCursorPosition(local.y());
}

void
DisplayView::updateActiveDisplayViewIndicator(bool on)
{
    static Logger::ProcLog log("updateActiveDisplayViewIndicator", Log());
    LOGINFO << this << " + " << visualizer_ << ' ' << on << std::endl;
    QPalette p(palette());
    p.setColor(QPalette::WindowText, on ? "Green" : "White");
    setPalette(p);
    update();
}

void
DisplayView::setActiveDisplayView()
{
    static Logger::ProcLog log("setActiveDisplayView", Log());
    LOGINFO << this << " + " << visualizer_ << std::endl;
    if (activeDisplayView_ != this) {
	if (activeDisplayView_)
	    activeDisplayView_->updateActiveDisplayViewIndicator(false);
	activeDisplayView_ = this;
	updateActiveDisplayViewIndicator(true);
	emit activeDisplayViewChanged(this);
    }
}

void
DisplayView::resizeEvent(QResizeEvent* evt)
{
    Super::resizeEvent(evt);
    updateActiveDisplayViewIndicator(activeDisplayView_ == this);
}

bool
DisplayView::isHorizontalScaleVisible() const
{
    return horizontalScale_->isVisible();
}

bool
DisplayView::isVerticalScaleVisible() const
{
    return verticalScale_->isVisible();
}

void
DisplayView::setHorizontalScaleVisibility(bool value)
{
    horizontalScale_->setVisible(value);
}

void
DisplayView::setVerticalScaleVisibility(bool value)
{
    verticalScale_->setVisible(value);
}

void
DisplayView::setFrozen(bool state)
{
    visualizer_->setFrozen(state);
}

void
DisplayView::setShowGrid(bool state)
{
    visualizer_->setShowGrid(state);
}

void
DisplayView::setShowPeakBars(bool state)
{
    visualizer_->setShowPeakBars(state);
}

bool
DisplayView::isFrozen() const
{
    return visualizer_->isFrozen();
}

bool
DisplayView::isShowingGrid() const
{
    return visualizer_->isShowingGrid();
}

bool
DisplayView::isShowingPeakBars() const
{
    return visualizer_->isShowingPeakBars();
}
