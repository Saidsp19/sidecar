#include "GUI/VideoSampleCountTransform.h"
#include "Utils/SineCosineLUT.h"

#include "App.h"
#include "Configuration.h"
#include "VideoDecimator.h"
#include "VideoImaging.h"
#include "VideoVertexGenerator.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::PPIDisplay;

VideoVertexGenerator::VideoVertexGenerator()
    : Super()
{
    Configuration* cfg = App::GetApp()->getConfiguration();
    imaging_ = cfg->getVideoImaging();
    transform_ = cfg->getVideoSampleCountTransform();
    sineCosineLUT_ = cfg->getSineCosineLUT();
    viewSettings_ = cfg->getViewSettings();
}

void
VideoVertexGenerator::renderMessage(const Messages::PRIMessage::Ref& msg, VertexColorArray& points)
{
    const Messages::Video::Ref video = boost::dynamic_pointer_cast<Messages::Video>(msg);

    points.checkCapacity(video->size());

    double sine;
    double cosine;
    sineCosineLUT_->lookup(msg->getShaftEncoding(), sine, cosine);
    double rangeMax = viewSettings_->getRangeMax();

    int decimation = imaging_->getDecimation();
    if (decimation > 1) {
	VideoDecimator decimator(decimation, video);
	while (decimator) {
	    double range = decimator.getRange();
	    if (range > rangeMax) break;
	    double intensity = transform_->transform(decimator.getValue());
	    points.push_back(Vertex(range * sine, range * cosine), imaging_->getColor(intensity));
	}
    }
    else {
	Messages::Video::const_iterator pos = video->begin();
	Messages::Video::const_iterator end = video->end();
	while (pos != end) {
	    double range = video->getRangeAt(pos);
	    if (range > rangeMax) break;
	    double intensity = transform_->transform(*pos++);
	    points.push_back(Vertex(range * sine, range * cosine), imaging_->getColor(intensity));
	}
    }
}
