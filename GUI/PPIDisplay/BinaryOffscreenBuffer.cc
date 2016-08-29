#include "GUI/SampleImaging.h"
#include "Messages/BinaryVideo.h"

#include "App.h"
#include "BinaryOffscreenBuffer.h"
#include "Configuration.h"

using namespace SideCar;
using namespace SideCar::GUI::PPIDisplay;

BinaryOffscreenBuffer::BinaryOffscreenBuffer(int width, int height,
                                             int textureType)
    : Super(App::GetApp()->getConfiguration()->getBinaryImaging(), width,
            height, textureType)
{
    ;
}

void
BinaryOffscreenBuffer::addPoints(const Messages::PRIMessage::Ref& msg)
{
    checkPointCapacity(msg->size());

    double sine;
    double cosine;
    getSineCosine(msg->getShaftEncoding(), sine, cosine);

    const SampleImaging* imaging(getImaging());
    GLfloat offset = imaging->getSize() / 2.0;
    offset = 0.0;

    Color on(imaging->getColor());
    Color off(0.0, 0.0, 0.0, 0.0);

    const Messages::BinaryVideo::Ref binary =
	boost::dynamic_pointer_cast<Messages::BinaryVideo>(msg);

    double rangeMax = getRangeMax();
    int decimation = imaging->getDecimation();
    if (decimation > 1) {
	BinaryDecimator decimator(decimation, binary);
	while (decimator) {
	    double range = decimator.getRange();
	    if (range > rangeMax)
		break;
	    addPoint(Vertex(range * sine - offset, range * cosine - offset),
                     decimator.getValue() ? on : off);
	}
    }
    else {
	Messages::BinaryVideo::const_iterator pos = binary->begin();
	Messages::BinaryVideo::const_iterator end = binary->end();
	while (pos != end) {
	    double range = binary->getRangeAt(pos);
	    if (range > rangeMax)
		break;
	    addPoint(Vertex(range * sine - offset, range * cosine - offset),
                     *pos++ ? on : off);
	}
    }
}
