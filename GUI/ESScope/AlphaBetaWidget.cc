#include "boost/bind.hpp"

#include "GUI/LogUtils.h"
#include "GUI/Texture.h"

#include "AlphaBetaPlotPositioner.h"
#include "AlphaBetaView.h"
#include "AlphaBetaWidget.h"
#include "RadarSettings.h"
#include "VideoImaging.h"
#include "VideoSampleCountTransform.h"

using namespace SideCar::GUI::ESScope;

Logger::Log&
AlphaBetaWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("esscope.AlphaBetaWidget");
    return log_;
}

AlphaBetaWidget::AlphaBetaWidget(AlphaBetaView* parent,
                                 ViewSettings* viewSettings)
    : Super(parent, viewSettings, new AlphaBetaPlotPositioner),
      lastAlpha_(0), lastBeta_(0)
{
    connect(history_,
            SIGNAL(currentMessage(const Messages::PRIMessage::Ref&)),
            SLOT(currentMessage(const Messages::PRIMessage::Ref&)));
    connect(radarSettings_, SIGNAL(betaMinMaxChanged(double, double)),
            SLOT(generateVertices()));
}

void
AlphaBetaWidget::paintGL()
{
    static Logger::ProcLog log("paintGL", Log());

    Super::paintGL();

    GLEC(glPointSize(6.0));
    GLEC(glColor3f(1.0, 1.0, 1.0));
    glBegin(GL_POINTS);
    glVertex2f(lastAlpha_, lastBeta_) ;
    glEnd();

    GLEC(glDisable(GL_BLEND));
}

void
AlphaBetaWidget::alphasChanged(const AlphaIndices& alphaIndices)
{
    static Logger::ProcLog log("alphasChanged", Log());
    colors_.clear();
    std::for_each(alphaIndices.begin(), alphaIndices.end(),
                  boost::bind(&AlphaBetaWidget::updateColumn, this, _1));
    updateColors(alphaIndices[0] * getYScans());
}

void
AlphaBetaWidget::updateColumn(int alphaIndex)
{
    size_t pos = alphaIndex * getYScans();
    size_t limit = pos + getYScans();
    while (pos < limit) {
	colors_.add(
	    videoImaging_->getColor(
		videoSampleCountTransform_->transform(
		    history_->getAlphaBetaValue(pos++))));
    }
}

void
AlphaBetaWidget::fillColors()
{
    for (int index = 0; index < gridSize_; ++index) {
	colors_.add(
	    videoImaging_->getColor(
		videoSampleCountTransform_->transform(
		    history_->getAlphaBetaValue(index))));
    }
}

void
AlphaBetaWidget::currentMessage(const Messages::PRIMessage::Ref& msg)
{
    lastAlpha_ = radarSettings_->getAlpha(msg);
    lastBeta_ = radarSettings_->getBeta(msg);
}
