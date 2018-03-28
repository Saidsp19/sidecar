#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/ChannelBuffer.h"
#include "Logger/Log.h"

#include "Summer.h"
#include "Summer_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
Summer::Summer(Controller& controller, Logger::Log& log) : Super(controller, log)
{
    ;
}

bool
Summer::processChannels()
{
    static Logger::ProcLog log("processChannels", getLog());
    LOGINFO << std::endl;

    // If we are not enabled, just copy the message from the first enabled channel to the output.
    //
    if (!isEnabled()) {
        for (size_t index = 0; index < getChannelCount(); ++index) {
            VideoChannelBuffer* channel = getChannelBuffer<Messages::Video>(index);
            if (channel->isEnabled()) {
                Messages::Video::Ref msg(channel->popFront());
                Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
                out->getData() = msg->getData();
                LOGDEBUG << "sending from channel " << channel->getChannelIndex() << std::endl;
                return send(out);
            }
        }

        LOGDEBUG << "nothing to output" << std::endl;
        return true;
    }

    using IteratorVector = std::vector<Messages::Video::const_iterator>;
    IteratorVector iterators;
    size_t size = 0;
    Messages::Video::Ref out;

    // Obtain iterators to the samples of the messages in the input buffers. NOTE: do not call channel->popFront()
    // since we need to keep a reference around for the message. Call popFront after we've built our output message.
    //
    for (size_t index = 0; index < getChannelCount(); ++index) {
        VideoChannelBuffer* channel = getChannelBuffer<Messages::Video>(index);
        if (channel->isEnabled()) {
            Messages::Video::Ref msg(channel->getFront());
            if (!out) {
                out = Messages::Video::Make(getName(), msg);
                size = msg->size();
            } else if (size > msg->size()) {
                size = msg->size();
            }
            iterators.push_back(msg->begin());
        }
    }

    if (!out) {
        LOGWARNING << "enabled but no channels enabled" << std::endl;
        return true;
    }

    while (size-- != 0) {
        IteratorVector::iterator pos = iterators.begin();
        Messages::Video::DatumType value = *(*pos++)++;
        IteratorVector::iterator end = iterators.end();
        while (pos != end) { value += *(*pos++)++; }
        out->push_back(value);
    }

    // Now we are done with the messages, drop them from the buffers.
    //
    for (size_t index = 0; index < getChannelCount(); ++index) {
        VideoChannelBuffer* channel = getChannelBuffer<Messages::Video>(index);
        if (channel->isEnabled()) { channel->popFront(); }
    }

    LOGDEBUG << out->headerPrinter() << std::endl;
    LOGDEBUG << out->dataPrinter() << std::endl;

    return send(out);
}

ChannelBuffer*
Summer::makeChannelBuffer(int channelIndex, const std::string& name, size_t maxBufferSize)
{
    VideoChannelBuffer* channel = new VideoChannelBuffer(*this, channelIndex, maxBufferSize);
    channel->makeEnabledParameter();
    return channel;
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    return Algorithm::FormatInfoValue(ManyInAlgorithm::GetFormattedStats(status));
}

// Factory function for the DLL that will create a new instance of the Summer class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
SummerMake(Controller& controller, Logger::Log& log)
{
    return new Summer(controller, log);
}
