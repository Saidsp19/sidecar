#include "Messages/BinaryVideo.h"

#include "App.h"
#include "BinaryOffscreenBuffer.h"
#include "Configuration.h"
#include "SampleImaging.h"
#include "ViewSettings.h"

using namespace SideCar;
using namespace SideCar::GUI::BScope;

BinaryOffscreenBuffer::BinaryOffscreenBuffer(int width, int height,
                                             int textureType)
    : Super(App::GetApp()->getConfiguration()->getBinaryImaging(), width,
            height, textureType)
{
    ;
}

class BinaryDecimator
{
public:
    BinaryDecimator(int factor, const Messages::BinaryVideo::Ref& msg)
	: factor_(factor),
	  rangeOffset_(factor / 2.0 * msg->getRangeFactor()),
	  msg_(msg), pos_(msg->begin()), end_(msg->end()) {}

    operator bool()
	{
	    if (pos_ == end_) return false;
	    range_ = msg_->getRangeAt(pos_) + rangeOffset_;
	    long sum = *pos_++;
	    for (int count = 1; pos_ < end_ && count < factor_; ++count)
		sum += *pos_++;
	    value_ = sum >= factor_;
	    return true;
	}

    double getRange() const { return range_; }
    bool getValue() const { return value_; }

private:
    int factor_;
    int shift_;
    double rangeOffset_;
    Messages::BinaryVideo::Ref msg_;
    Messages::BinaryVideo::const_iterator pos_;
    Messages::BinaryVideo::const_iterator end_;
    double range_;
    bool value_;
};

void
BinaryOffscreenBuffer::addPoints(const Messages::PRIMessage::Ref& msg)
{
    checkPointCapacity(msg->size());
    double azimuth =
	getViewSettings()->normalizedAzimuth(msg->getAzimuthStart());

    const Messages::BinaryVideo::Ref binary =
	boost::dynamic_pointer_cast<Messages::BinaryVideo>(msg);
    const SampleImaging* imaging(getImaging());
    Color on(imaging->getColor());
    Color off(0.0, 0.0, 0.0, 0.0);
    double rangeMin = getRangeMin();
    double rangeMax = getRangeMax();
    int decimation = imaging->getDecimation();
    if (decimation > 1) {
	BinaryDecimator decimator(imaging->getDecimation(), binary);
	while (decimator) {
	    double range = decimator.getRange();
	    if (range > rangeMax)
		break;
	    if (range >= rangeMin)
		addPoint(Vertex(azimuth, range),
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
	    if (range >= rangeMin) {
		addPoint(Vertex(azimuth, range), *pos ? on : off);
	    }
	    ++pos;
	}
    }
}
