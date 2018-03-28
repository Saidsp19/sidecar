#include <iostream>

#include "App.h"
#include "ChannelConnection.h"
#include "ConfigurationWindow.h"
#include "PeakBarCollection.h"
#include "PeakBarSettings.h"
#include "RenderInfo.h"
#include "Visualizer.h"

using namespace SideCar::GUI::AScope;

RenderInfo::RenderInfo(Visualizer& visualizer, VideoChannel& channel, bool visible, bool showPeakBars) :
    QObject(), visualizer_(visualizer), peakBars_(channel.getPeakBars()), historySlot_(channel.displayAdded()),
    color_(channel.getColor()), visible_(visible), showPeakBars_(showPeakBars), frozen_(false), lastRendered_(),
    plotPoints_(), barSize_(-1, -1), origin_(-1, -1), peakBarPoints_()
{
    connect(&channel, SIGNAL(colorChanged(const QColor&)), SLOT(setColor(const QColor&)));

    updatePeakBarCache();

    connect(&visualizer, SIGNAL(transformChanged()), SLOT(visualizerTransformChanged()));
    connect(&peakBars_, SIGNAL(transformChanged()), SLOT(updatePeakBarCache()));

    PeakBarSettings* peakBarSettings = App::GetApp()->getPeakBarSettings();
    connect(peakBarSettings, SIGNAL(widthChanged()), SLOT(adjustPeakBars()));
    connect(peakBarSettings, SIGNAL(fadingChanged()), &visualizer, SLOT(update()));
    connect(peakBarSettings, SIGNAL(enabledChanged(bool)), &visualizer, SLOT(update()));
}

void
RenderInfo::setColor(const QColor& color)
{
    color_ = color;
    visualizer_.update();
}

void
RenderInfo::setVisible(bool visible)
{
    visible_ = visible;
    visualizer_.update();
}

void
RenderInfo::setFrozen(bool frozen)
{
    frozen_ = frozen;
}

void
RenderInfo::setShowPeakBars(bool showPeakBars)
{
    showPeakBars_ = showPeakBars;
    visualizer_.needUpdate();
}

void
RenderInfo::visualizerTransformChanged()
{
    lastRendered_.reset();
    updatePeakBarCache();
}

void
RenderInfo::updatePeakBarCache()
{
    peakBars_->updateRenderInfo(*this, visualizer_.getTransform());
    visualizer_.needUpdate();
}

void
RenderInfo::adjustPeakBars()
{
    peakBars_->adjustBars(true);
    updatePeakBarCache();
    visualizer_.needUpdate();
}
