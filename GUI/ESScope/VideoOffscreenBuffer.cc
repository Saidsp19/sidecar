#include "GUI/VideoSampleCountTransform.h"
#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "VideoImaging.h"
#include "VideoOffscreenBuffer.h"
#include "ViewSettings.h"

using namespace SideCar;
using namespace SideCar::GUI::ESScope;

VideoOffscreenBuffer::VideoOffscreenBuffer() : Super(App::GetApp()->getConfiguration()->getVideoImaging())
{
    transform_ = App::GetApp()->getConfiguration()->getVideoSampleCountTransform();
}

class VideoDecimator {
public:
    VideoDecimator(int factor, const Messages::Video::Ref& msg) :
        factor_(factor), rangeOffset_(factor / 2.0 * msg->getRangeFactor()), msg_(msg), pos_(msg->begin()),
        end_(msg->end())
    {
    }

    operator bool()
    {
        if (pos_ == end_) return false;
        range_ = msg_->getRangeAt(pos_) + rangeOffset_;
        long sum = *pos_++;
        for (int count = 1; pos_ < end_ && count < factor_; ++count) sum += *pos_++;
        value_ = sum / factor_;
        return true;
    }

    double getRange() const { return range_; }
    int getValue() const { return value_; }

private:
    int factor_;
    int shift_;
    double rangeOffset_;
    Messages::Video::Ref msg_;
    Messages::Video::const_iterator pos_;
    Messages::Video::const_iterator end_;
    double range_;
    int value_;
};

const VideoImaging*
VideoOffscreenBuffer::getImaging() const
{
    return static_cast<const VideoImaging*>(Super::getImaging());
}

void
VideoOffscreenBuffer::addPoints(const Messages::PRIMessage::Ref& msg)
{
    checkPointCapacity(msg->size());
    double azimuth = getViewSettings()->normalizedAzimuth(msg->getAzimuthStart());

    const Messages::Video::Ref video = boost::dynamic_pointer_cast<Messages::Video>(msg);
    const VideoImaging* imaging = getImaging();
    double rangeMin = getRangeMin();
    double rangeMax = getRangeMax();
    int decimation = imaging->getDecimation();
    if (decimation > 1) {
        VideoDecimator decimator(decimation, video);
        while (decimator) {
            double range = decimator.getRange();
            if (range > rangeMax) break;
            if (range >= rangeMin) {
                double intensity = transform_->transform(decimator.getValue());
                addPoint(Vertex(azimuth, range), imaging->getColor(intensity));
            }
        }
    } else {
        Messages::Video::const_iterator pos = video->begin();
        Messages::Video::const_iterator end = video->end();
        while (pos != end) {
            double range = video->getRangeAt(pos);
            if (range > rangeMax) break;
            if (range >= rangeMin) {
                double intensity = transform_->transform(*pos);
                addPoint(Vertex(azimuth, range), imaging->getColor(intensity));
            }
            ++pos;
        }
    }
}
