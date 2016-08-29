#include <cmath>

#include "QtGui/QGridLayout"

#include "GUI/LogUtils.h"
#include "GUI/RangeRingsImaging.h"

#include "App.h"
#include "Configuration.h"
#include "FrameWidget.h"
#include "PastImage.h"
#include "ScaleWidget.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::BScope;

Logger::Log&
FrameWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.FrameWidget");
    return log_;
}

FrameWidget::FrameWidget(QWidget* parent, const QImage& image,
                         const QSize& size)
    : Super(parent)
{
    Logger::ProcLog log("FrameWidget", Log());
    LOGINFO << "size: " << image.size() << std::endl;

    // setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);

    frame_ = new PastImage(this, image, size);
    layout->addWidget(frame_, 0, 1, 1, 1);

    azimuthScale_ = new DegreesScaleWidget(this, Qt::Horizontal);
    azimuthScale_->setMajorTickDivisions(4);
    layout->addWidget(azimuthScale_, 1, 1);

    rangeScale_ = new ScaleWidget(this, Qt::Vertical);
    rangeScale_->setMajorTickDivisions(4);

    layout->addWidget(rangeScale_, 0, 0, 1, 1);

    App* app = App::GetApp();
    connect(app, SIGNAL(phantomCursorChanged(const QPointF&)),
            SLOT(setPhantomCursor(const QPointF&)));
    RangeRingsImaging* imaging =
	app->getConfiguration()->getRangeRingsImaging();
    connect(imaging, SIGNAL(tickSettingsChanged()),
            SLOT(updateScaleTicks()));
    updateScaleTicks();

    ViewSettings* viewSettings = 
	app->getConfiguration()->getViewSettings();
    connect(viewSettings, SIGNAL(settingChanged()),
            SLOT(updateScaleRanges()));

    updateScaleRanges();
}

void
FrameWidget::updateScaleTicks()
{
    ViewSettings* viewSettings =
	App::GetApp()->getConfiguration()->getViewSettings();
    RangeRingsImaging* imaging =
	App::GetApp()->getConfiguration()->getRangeRingsImaging();
    
    int azimuthMajorTicks = int(
	::rint(360.0 / imaging->getAzimuthSpacing()));
    azimuthScale_->setMajorAndMinorTickDivisions(azimuthMajorTicks,
                                                 imaging->getAzimuthTicks());

    int rangeMajorTicks = int(
	::rint(viewSettings->getRangeMax() / imaging->getRangeSpacing()));
    rangeScale_->setMajorAndMinorTickDivisions(rangeMajorTicks,
                                               imaging->getRangeTicks());
}    

void
FrameWidget::updateScaleRanges()
{
    ViewSettings* viewSettings =
	App::GetApp()->getConfiguration()->getViewSettings();
    azimuthScale_->setStartAndEnd(viewSettings->getAzimuthMin(),
                                  viewSettings->getAzimuthMax());
    rangeScale_->setStartAndEnd(viewSettings->getRangeMin(),
                                viewSettings->getRangeMax());
}

void
FrameWidget::updateMinMaxSizes(const QSize& oldSize)
{
    QSize delta(frame_->size());
    delta -= oldSize;

    // Inhibit min/max size checking while we resize.
    //
    layout()->setSizeConstraint(QLayout::SetNoConstraint);
    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    resize(size() + delta);

    // Let the layout resume control of our sizing.
    //
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    updateGeometry();
}

void
FrameWidget::setImage(const QImage& image)
{
    QSize oldSize(frame_->size());
    frame_->setImage(image);
    if (frame_->size() != oldSize) {
	updateMinMaxSizes(oldSize);
    }
}

void
FrameWidget::setImageSize(const QSize& size)
{
    QSize oldSize(frame_->size());
    frame_->setImageSize(size);
    if (frame_->size() != oldSize) {
	updateMinMaxSizes(oldSize);
    }
}

void
FrameWidget::setPhantomCursor(const QPointF& pos)
{
    frame_->setPhantomCursor(pos);
    azimuthScale_->setNormalizedCursorPosition(pos.x());
    rangeScale_->setNormalizedCursorPosition(pos.y());
}
