#include "boost/bind.hpp"

#include "GUI/LogUtils.h"
#include "GUI/Texture.h"

#include "AlphaRangePlotPositioner.h"
#include "AlphaRangeView.h"
#include "AlphaRangeWidget.h"
#include "RadarSettings.h"
#include "VideoImaging.h"
#include "VideoSampleCountTransform.h"

using namespace SideCar::GUI::ESScope;

Logger::Log&
AlphaRangeWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("esscope.AlphaRangeWidget");
    return log_;
}

AlphaRangeWidget::AlphaRangeWidget(AlphaRangeView* parent, ViewSettings* viewSettings) :
    Super(parent, viewSettings, new AlphaRangePlotPositioner)
{
    connect(radarSettings_, SIGNAL(rangeMinMaxChanged(double, double)), SLOT(generateVertices()));
    connect(radarSettings_, SIGNAL(rangeMinMaxChanged(double, double)), SLOT(generateVertices()));
    connect(radarSettings_, SIGNAL(rangeScalingChanged()), SLOT(clear()));
}

void
AlphaRangeWidget::paintGL()
{
    static Logger::ProcLog log("paintGL", Log());

    Super::paintGL();

    double firstSampleRange = radarSettings_->getRange(radarSettings_->getFirstSample());
    double lastSampleRange = radarSettings_->getRange(radarSettings_->getLastSample());

    if (firstSampleRange == lastSampleRange) return;

    GLEC(glColor4f(1.0, 0.0, 0.0, 0.25));
    glBegin(GL_QUADS);
    glVertex2f(getXMinMin(), getYMinMin());
    glVertex2f(getXMaxMax(), getYMinMin());
    glVertex2f(getXMaxMax(), firstSampleRange);
    glVertex2f(getXMinMin(), firstSampleRange);

    glVertex2f(getXMinMin(), getYMaxMax());
    glVertex2f(getXMaxMax(), getYMaxMax());
    glVertex2f(getXMaxMax(), lastSampleRange);
    glVertex2f(getXMinMin(), lastSampleRange);
    glEnd();

    GLEC(glDisable(GL_BLEND));
}

void
AlphaRangeWidget::alphasChanged(const AlphaIndices& alphaIndices)
{
    static Logger::ProcLog log("alphasChanged", Log());
    colors_.clear();
    std::for_each(alphaIndices.begin(), alphaIndices.end(), boost::bind(&AlphaRangeWidget::updateColumn, this, _1));
    updateColors(alphaIndices[0] * getYScans());
}

void
AlphaRangeWidget::updateColumn(int alphaIndex)
{
    const AlphaRangeColumn& samples(history_->getAlphaRangeData(alphaIndex));
    int limit = std::min(getYScans(), int(samples.size()));
    for (int sampleIndex = 0; sampleIndex < limit; ++sampleIndex) {
        int value = samples[sampleIndex];
        colors_.add(videoImaging_->getColor(videoSampleCountTransform_->transform(value)));
    }

    while (limit++ < getYScans()) colors_.add(clearColor_);
}

void
AlphaRangeWidget::fillColors()
{
    // Revisit all columns of range values and recalculate their colors.
    //
    for (int x = 0; x < getXScans(); ++x) {
        const AlphaRangeColumn& samples(history_->getAlphaRangeData(x));

        // Make sure we don't over do it.
        //
        int limit = std::min(getYScans(), int(samples.size()));

        // Visit each sample in range, calculate its color value and add to the color container.
        //
        for (int y = 0; y < limit; ++y) {
            int value = samples[y];
            colors_.add(videoImaging_->getColor(videoSampleCountTransform_->transform(value)));
        }

        // Make sure the color container stays aligned with the vertex buffer.
        //
        while (limit++ < getYScans()) colors_.add(clearColor_);
    }
}
