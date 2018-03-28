#include "GUI/SampleImaging.h"

#include "App.h"
#include "BinaryDecimator.h"
#include "BinaryVertexGenerator.h"
#include "Configuration.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::BScope;

BinaryVertexGenerator::BinaryVertexGenerator() : Super()
{
    Configuration* cfg = App::GetApp()->getConfiguration();
    imaging_ = cfg->getBinaryImaging();
    viewSettings_ = cfg->getViewSettings();
}

void
BinaryVertexGenerator::renderMessage(const Messages::PRIMessage::Ref& msg, VertexColorArray& points)
{
    const Messages::BinaryVideo::Ref binary = boost::dynamic_pointer_cast<Messages::BinaryVideo>(msg);

    points.checkCapacity(binary->size());

    double azimuth = viewSettings_->normalizedAzimuth(msg->getAzimuthStart());
    Color on(imaging_->getColor());
    Color off(0.0, 0.0, 0.0, 0.0);

    int decimation = imaging_->getDecimation();
    if (decimation > 1) {
        BinaryDecimator decimator(decimation, binary);
        while (decimator) {
            double range = decimator.getRange();
            points.push_back(Vertex(azimuth, range), decimator.getValue() ? on : off);
        }
    } else {
        Messages::BinaryVideo::const_iterator pos = binary->begin();
        Messages::BinaryVideo::const_iterator end = binary->end();
        while (pos != end) {
            double range = binary->getRangeAt(pos);
            points.push_back(Vertex(azimuth, range), *pos++ ? on : off);
        }
    }
}
