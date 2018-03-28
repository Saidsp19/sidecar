#include "GUI/LogUtils.h"
#include "GUI/SampleImaging.h"
#include "Utils/SineCosineLUT.h"

#include "App.h"
#include "BinaryDecimator.h"
#include "BinaryVertexGenerator.h"
#include "Configuration.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::PPIDisplay;

Logger::Log&
BinaryVertexGenerator::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.BinaryVertexGenerator");
    return log_;
}

BinaryVertexGenerator::BinaryVertexGenerator() : Super()
{
    Configuration* cfg = App::GetApp()->getConfiguration();
    imaging_ = cfg->getBinaryImaging();
    sineCosineLUT_ = cfg->getSineCosineLUT();
    viewSettings_ = cfg->getViewSettings();
}

void
BinaryVertexGenerator::renderMessage(const Messages::PRIMessage::Ref& msg, VertexColorArray& points)
{
    static Logger::ProcLog log("renderMessage", Log());

    const Messages::BinaryVideo::Ref video = boost::dynamic_pointer_cast<Messages::BinaryVideo>(msg);

    points.checkCapacity(video->size());

    double sine;
    double cosine;
    sineCosineLUT_->lookup(msg->getShaftEncoding(), sine, cosine);
    double rangeMax = viewSettings_->getRangeMax();

    Color on(imaging_->getColor());
    Color off(0.0, 0.0, 0.0, 0.0);

    int decimation = imaging_->getDecimation();
    if (decimation > 1) {
        BinaryDecimator decimator(decimation, video);
        while (decimator) {
            double range = decimator.getRange();
            if (range > rangeMax) break;
            points.push_back(Vertex(range * sine, range * cosine), decimator.getValue() ? on : off);
        }
    } else {
        Messages::BinaryVideo::const_iterator pos = video->begin();
        Messages::BinaryVideo::const_iterator end = video->end();
        while (pos != end) {
            double range = video->getRangeAt(pos);
            if (range > rangeMax) break;
            points.push_back(Vertex(range * sine, range * cosine), *pos ? on : off);
            ++pos;
        }
    }
}
