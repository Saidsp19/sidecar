#ifndef SIDECAR_ALGORITHMS_MANYINALGORITHM_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_MANYINALGORITHM_H

#include <vector>
#include <limits>

#include "QtCore/QString"

#include "Algorithms/Algorithm.h"
#include "Algorithms/ControllerStatus.h"

namespace SideCar {
namespace Algorithms {

class ChannelBuffer;
template <typename T> class TChannelBuffer;

/** Derivation of the basic Algorithm class that handles multiple input channels using ChannelBuffer objects.
    Often when processing multiple input channels, one needs to obtain a message from all of the channels with
    the same sequence number. The getMaxSequenceCounter() method will return the next valid sequence counter
    common to all channels. The return value may then be used in a call to pruneToSequenceCounter(), which will
    remove from the channels any messages with a lesser sequence counter value, returning true if one or more of
    the resulting channel buffers is empty. When it returns false, then one may safely access the first message
    from each ChannelBuffer object using ChannelBuffer::getFront().
*/
class ManyInAlgorithm : public Algorithm
{
    using Super = Algorithm;
public:

    enum InfoSlot {
	kEnabled = ControllerStatus::kNumSlots,
	kChannelStats,
	kNumSlots
    };

    static QString GetFormattedStats(const IO::StatusBase& status);

    /** Constructor. 

        \param controller the controller managing our instance

        \param log the log device to use for log messages

        \param enabled sets initial state of enabled_ attribute (default is true)

        \param maxBufferSize sets initial message buffer size (default is 10)
    */
    ManyInAlgorithm(Controller& controller, Logger::Log& log, bool enabled = true, size_t maxBufferSize = 10);

    /** Override of Algorithm::startup(). Calls makeChannelBuffer() method for each input channel defined for
        the algorithm. Derived classes are responsible for implementing makeChannelBuffer(). Registers the
        maxBufferSize and enabled runtime parameters.

        \return true if successful
    */
    bool startup() override;

    /** Override of Algorithm::shutdown(). Disposes of ChannelBuffer objects created in the startup() method.

	\return true if successful
    */
    bool shutdown() override;

    /** Obtain enabled state for the algorithm.

        \return true if enabled
    */
    bool isEnabled() const { return enabled_->getValue(); }

    /** Obtain the current max buffer size.

        \return max buffer size
    */
    size_t getMaxBufferSize() const { return maxBufferSize_->getValue(); }

    /** Obtain the number of active ChannelBuffer objects

        \return number of active ChannelBuffer objects
    */
    size_t getChannelCount() const { return channels_.size(); }

    /** Obtain a type-safe ChannelBuffer object using the channel name.

        \param name the name of the channel to obtain

        \return found channel or NULL if error
    */
    template <typename T>
    TChannelBuffer<T>* getChannelBuffer(const std::string& name) const;

    /** Obtain a type-safe ChannelBuffer object using the channel index.

        \param index the index of the channel to obtain

        \return found channel
    */
    template <typename T>
    TChannelBuffer<T>* getChannelBuffer(size_t index) const;

    /** Obtain the ChannelBuffer object at a given channel index.

        \param index the channel to fetch

        \return ChannelBuffer object
    */
    ChannelBuffer* getGenericChannelBuffer(size_t index) const
	{ return channels_[index]; }

    bool reset() override;

    /** Process the ChannelBuffer objects. Called whenever an active ChannelBuffer object receives a message.

        \return true if successful
    */
    virtual bool processMessageReceived();

    /** Notification handler invoked when the enabled state of a ChannelBuffer object changes. This
        implementation does nothing.

        \param channel the ChannelBuffer object that changed
    */
    virtual void channelEnabledChanged(ChannelBuffer* channel);

protected:

    /** Obtain the number of info slots defined by this algorithm.

        \return info slot count
    */
    size_t getNumInfoSlots() const override { return kNumSlots; }

    /** Override of Algorithm::setInfoSlots(). Stores XML representation of channel stats into the given XML-RPC
        container.

        \param status XML-RPC container to hold the stats
    */
    void setInfoSlots(IO::StatusBase& status) override;

    /** Obtain the maximum sequence counter seen in the first message of all of the ChannelBuffer objects.

        \return maximum sequence counter
    */
    uint32_t getMaxSequenceCounter() const;

    /** Prune the active ChannelBuffer objects so that the first message in each one has the given sequence
        counter value. This operation may result in an empty ChannelBuffer object if it does not have a message
        with the given sequence counter, in which case the routine will return true.

        \param sequenceCounter the value to look for

        \return true if one or more of the ChannelBuffers is empty after the
        pruning
    */
    bool pruneToSequenceCounter(uint32_t sequenceCounter);

    /** Process messages from all of the channels. Only called when all enabled channels have a message with the
        same sequence number.

        \return true if successful
    */
    virtual bool processChannels() = 0;

    /** Create a new type-safe TChannelBuffer object. Derived classes must define.

        \param channelIndex the index of the channel to define

        \param name the name of the channel to define

        \param maxBufferSize the max size for the buffer

        \return TChannelBuffer object
    */
    virtual ChannelBuffer* makeChannelBuffer(int channelIndex,
                                             const std::string& name,
                                             size_t maxBufferSize) = 0;

    ChannelBuffer* findChannelBuffer(const std::string& name) const;

    using ChannelBufferVector = std::vector<ChannelBuffer*>;
    ChannelBufferVector channels_;

    template <typename T>
    size_t fetchEnabledChannelMessages(std::vector<typename T::Ref>& inputs) {
	size_t minSize = std::numeric_limits<size_t>::max();
	for (size_t index = 0; index < getChannelCount(); ++index) {
	    TChannelBuffer<T>* channel = getChannelBuffer<T>(index);
	    if (channel->isEnabled()) {
		inputs.push_back(channel->popFront());
		size_t size = inputs.back()->size();
		if (size < minSize) minSize = size;
	    }
	}
	return minSize;
    }

private:

    /** Notification handler called when the maxBufferSize parameter changed. Invokes
        Channel::setMaxBufferSize() on all active Channel objects.

        \param parameter reference to parameter that changed
    */
    void maxBufferSizeChanged(const Parameter::PositiveIntValue& parameter);

    /** Run-time parameter for enabled state. If false, just pass thru messages from the first input channel.
     */
    Parameter::BoolValue::Ref enabled_;

    /** Run-time parameter for a channel's internal buffer size
     */
    Parameter::PositiveIntValue::Ref maxBufferSize_;
};

} // end namespace Algorithms
} // end namespace SideCar

#include "Algorithms/ChannelBuffer.h"

namespace SideCar {
namespace Algorithms {

template <typename T>
TChannelBuffer<T>*
ManyInAlgorithm::getChannelBuffer(const std::string& n) const
{
    return dynamic_cast<TChannelBuffer<T>*>(findChannelBuffer(n));
}

template <typename T>
TChannelBuffer<T>*
ManyInAlgorithm::getChannelBuffer(size_t index) const
{
    return dynamic_cast<TChannelBuffer<T>*>(channels_[index]);
}

} // end namespace Algorithms
} // end namespace SideCar

#endif
