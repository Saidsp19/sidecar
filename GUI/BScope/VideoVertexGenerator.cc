#include "GUI/VideoSampleCountTransform.h"

#include "App.h"
#include "Configuration.h"
#include "VideoDecimator.h"
#include "VideoImaging.h"
#include "VideoVertexGenerator.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::BScope;

VideoVertexGenerator::VideoVertexGenerator()
    : Super()
{
    Configuration* cfg = App::GetApp()->getConfiguration();
    imaging_ = cfg->getVideoImaging();
    transform_ = cfg->getVideoSampleCountTransform();
    viewSettings_ = cfg->getViewSettings();
}

void
VideoVertexGenerator::renderMessage(const Messages::PRIMessage::Ref& msg,
                                    VertexColorArray& points)
{
    const Messages::Video::Ref video =
	boost::dynamic_pointer_cast<Messages::Video>(msg);
    points.checkCapacity(video->size());

    double azimuth = viewSettings_->normalizedAzimuth(msg->getAzimuthStart());

    int decimation = imaging_->getDecimation();
    if (decimation > 1) {
	VideoDecimator decimator(decimation, video);
	while (decimator) {
	    double range = decimator.getRange();
	    double intensity = transform_->transform(decimator.getValue());
	    points.push_back(Vertex(azimuth, range),
                             imaging_->getColor(intensity));
	}
    }
    else {
	Messages::Video::const_iterator pos = video->begin();
	Messages::Video::const_iterator end = video->end();
	while (pos != end) {
	    double range = video->getRangeAt(pos);
	    double intensity = transform_->transform(*pos++);
	    points.push_back(Vertex(azimuth, range),
                             imaging_->getColor(intensity));
	}
    }
}
