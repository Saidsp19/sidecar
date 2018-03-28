#include "Logger/Log.h"
#include "Utils/Utils.h"

#include "Algorithm.h"

using namespace SideCar::Algorithms;

Algorithm::Algorithm(Controller& controller, Logger::Log& log) :
    controller_(controller), log_(log), processors_(), vpp_()
{
    ;
}

Algorithm::~Algorithm()
{
    std::for_each(processors_.begin(), processors_.end(), [](auto p) { delete p; });
}

size_t
Algorithm::addProcessor(size_t index, const Messages::MetaTypeInfo& metaTypeInfo, Processor* processor)
{
    Logger::ProcLog log("addProcessor", log_);
    LOGINFO << index << ' ' << metaTypeInfo.getName() << ' ' << processor << std::endl;

    // Verify that the channel is configured properly by comparing type keys.
    //
    const IO::Channel& channel(controller_.getInputChannel(index));
    if (metaTypeInfo.getKey() != channel.getTypeKey()) {
        delete processor;
        Utils::Exception ex("processor type mismatch - ");
        ex << metaTypeInfo.getName() << " != " << channel.getTypeName();
        log.thrower(ex);
    }

    // Expand the container if necessary. Catch duplicate settings.
    //
    if (index >= processors_.size()) {
        processors_.resize(index + 1);
    } else if (processors_[index]) {
        delete processor;
        Utils::Exception ex("processor already registered for type ");
        ex << metaTypeInfo.getName();
        log.thrower(ex);
    }

    // Finally, install
    //
    LOGDEBUG << "installing processor in slot " << index << std::endl;
    processors_[index] = processor;

    return index;
}

size_t
Algorithm::addProcessor(const std::string& name, const Messages::MetaTypeInfo& metaTypeInfo, Processor* processor)
{
    Logger::ProcLog log("addProcessor", log_);
    LOGINFO << name << ' ' << metaTypeInfo.getName() << ' ' << processor << std::endl;
    auto index = controller_.getInputChannelIndex(name);
    return addProcessor(index, metaTypeInfo, processor);
}

size_t
Algorithm::addProcessor(const Messages::MetaTypeInfo& metaTypeInfo, Processor* processor)
{
    Logger::ProcLog log("addProcessor", log_);
    LOGINFO << metaTypeInfo.getName() << ' ' << processor << std::endl;

    // If there are no input channels defined, install the processor in the
    // first slot. However, do not allow more than one definition.
    //
    auto numChannels = controller_.getNumInputChannels();
    if (numChannels == 0) {
        if (processors_.size()) {
            delete processor;
            Utils::Exception ex("processor already registered");
            log.thrower(ex);
        }
        processors_.push_back(processor);
        return 0;
    }

    // Search for the first input channel with the given type.
    //
    for (auto index = 0; index < numChannels; ++index) {
        const IO::Channel& channel(controller_.getInputChannel(index));
        if (channel.getTypeKey() == metaTypeInfo.getKey()) {
            // Attempt to add the processor at the given index.
            //
            return addProcessor(index, metaTypeInfo, processor);
        }
    }

    delete processor;
    Utils::Exception ex("channel with type '");
    ex << metaTypeInfo.getName() << "' does not exist for algorithm " << controller_.getAlgorithmName();
    log.thrower(ex);
    return 0;
}

void
Algorithm::removeProcessor(size_t index)
{
    Processor* tmp = processors_[index];
    processors_[index] = 0;
    delete tmp;
}

bool
Algorithm::process(const Messages::Header::Ref& msg, size_t channelIndex)
{
    Logger::ProcLog log("process", log_);
    LOGTIN << msg->getMetaTypeInfo().getName() << ' ' << channelIndex << std::endl;
    if (channelIndex >= processors_.size() || !processors_[channelIndex]) {
        Utils::Exception ex("no processor registered for channel ");
        ex << channelIndex;
        log.thrower(ex);
    }

    activeChannelIndex_ = channelIndex;
    auto rc = processors_[channelIndex]->process(msg);
    LOGTOUT << rc << std::endl;
    return rc;
}

bool
Algorithm::send(const Messages::Header::Ref& msg, size_t channelIndex)
{
    static Logger::ProcLog log("send", log_);
    LOGTIN << "channelIndex: " << channelIndex << std::endl;

    // Drop on the floor messages that don't go anywhere. NOTE: but don't do so if there are NO output channels,
    // since there are test cases which don't properly setup channel connections. *FIXME*
    //
    if (channelIndex && channelIndex == controller_.getNumOutputChannels()) {
        LOGTOUT << "NOOP" << std::endl;
        return true;
    }

    // Make sure we have a valid sequence counter for a given channelIndex.
    //
    while (channelIndex >= sequenceNumbers_.size()) {
        LOGDEBUG << "expanding sequenceNumber vector" << std::endl;
        sequenceNumbers_.push_back(0);
    }

    // Set the message sequence counter.
    //
    msg->setMessageSequenceNumber(++sequenceNumbers_[channelIndex]);

    // Hand message to the controller for delivery.
    //
    auto rc = controller_.send(msg, channelIndex);
    LOGTOUT << rc << std::endl;
    return rc;
}

void
Algorithm::setAlarm(int timerSecs)
{
    Logger::ProcLog log("setTimerSecs", log_);
    LOGTIN << "timerSecs: " << timerSecs << std::endl;
    controller_.setTimerSecs(timerSecs);
}

void*
Algorithm::FormatInfoValue(const QString& value)
{
    if (value.size() == 0) return nullptr;
    QByteArray ba(value.toUtf8());
    auto buffer = new char[ba.size() + 1];
    strcpy(buffer, ba.data());
    return buffer;
}

void*
Algorithm::FormatInfoValue(const char* value)
{
    size_t len;
    if (!value || (len = strlen(value)) == 0) return nullptr;
    auto buffer = new char[len + 1];
    strcpy(buffer, value);
    return buffer;
}
