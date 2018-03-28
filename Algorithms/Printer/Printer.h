#ifndef SIDECAR_ALGORITHMS_PRINTER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_PRINTER_H

#include "Algorithms/ManyInAlgorithm.h"
#include "Messages/BinaryVideo.h"
#include "Messages/PRIMessage.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

class MessagePrinter {
public:
    MessagePrinter(const std::string& channelName) : channelName_(channelName) {}

    /** Destructor. Here to silence irritating warnings from certain GNU compilers.
     */
    virtual ~MessagePrinter() {}

    const std::string& getChannelName() const { return channelName_; }

    /** Process a single SideCar message. Derived classes must define.

        \param name the channel name

        \param msg the message to process

        \param offset the index of the first sample to print

        \param count the number of samples to print
    */
    virtual void print(const Messages::Header::Ref& msg, int offset, int count) = 0;

private:
    std::string channelName_;
};

template <typename P, typename M>
class TMessagePrinter : public MessagePrinter {
public:
    using Proc = boost::function<void(P*, const std::string&, typename M::Ref, int, int)>;
    TMessagePrinter(const std::string& channelName, P* obj, Proc proc) :
        MessagePrinter(channelName), obj_(obj), proc_(proc)
    {
    }

    void print(const Messages::Header::Ref& msg, int offset, int count)
    {
        proc_(obj_, getChannelName(), boost::dynamic_pointer_cast<M>(msg), offset, count);
    }

private:
    P* obj_;
    Proc proc_;
};

/** Documentation for the algorithm Printer. Please describe what the algorithm does, in layman's terms and, if
    possible, mathematical terms.
*/
class Printer : public ManyInAlgorithm {
    using Super = ManyInAlgorithm;

public:
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Printer(Controller& controller, Logger::Log& log);

    bool startup();

private:
    /** Create a new ChannelBuffer object for Video/Binary messages. Also creates and registers a BoolParameter
        object for runtime editing of the enabled state of the ChannelBuffer object.

        \param channelIndex the index of the channel to create

        \param maxBufferSize the maximum number of messages to buffer

        \return new ChannelBuffer object
    */
    ChannelBuffer* makeChannelBuffer(int channelIndex, const std::string& name, size_t maxBufferSize);

    /** Implementation of ManyInAlgorithm::processChannels() method. Creates a mesage with summed sample values
        and emits it.

        \return
    */
    bool processChannels();

    void printHeader(const Messages::PRIMessage::Ref& msg);

    void printChannelName(const std::string& channelName);

    void printVideo(const std::string& name, const Messages::Video::Ref& msg, int offset, int count);

    void printBinaryVideo(const std::string& name, const Messages::BinaryVideo::Ref& msg, int offset, int count);

    Parameter::BoolValue::Ref isIQ_;
    Parameter::NonNegativeIntValue::Ref startBin_;
    Parameter::PositiveIntValue::Ref count_;

    std::vector<MessagePrinter*> printers_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
