#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*
#include <iomanip>

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "Printer.h"
#include "Printer_defaults.h"

#include "QtCore/QString"

#include <iostream>
#include <stdio.h>

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
Printer::Printer(Controller& controller, Logger::Log& log) :
    Super(controller, log, kDefaultEnabled, kDefaultMaxBufferSize),
    isIQ_(Parameter::BoolValue::Make("isIQ", "Is the data complex", kDefaultIsIQ)),
    startBin_(Parameter::NonNegativeIntValue::Make("binStart", "Index of first sample to print", kDefaultBinStart)),
    count_(Parameter::PositiveIntValue::Make("binCount", "Number of samples to print", kDefaultBinCount))
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// Printer class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
Printer::startup()
{
    return Super::startup() && registerParameter(startBin_) && registerParameter(count_) && registerParameter(isIQ_);
}

ChannelBuffer*
Printer::makeChannelBuffer(int channelIndex, const std::string& name, size_t maxBufferSize)
{
    const IO::Channel& channel(getController().getInputChannel(channelIndex));

    switch (channel.getTypeKey()) {
    case Messages::MetaTypeInfo::Value::kVideo:
        printers_.push_back(
            new TMessagePrinter<Printer, Messages::Video>(channel.getName(), this, &Printer::printVideo));
        return new TChannelBuffer<Messages::Video>(*this, channelIndex, maxBufferSize);
        break;

    case Messages::MetaTypeInfo::Value::kBinaryVideo:
        printers_.push_back(
            new TMessagePrinter<Printer, Messages::BinaryVideo>(channel.getName(), this, &Printer::printBinaryVideo));
        return new TChannelBuffer<Messages::BinaryVideo>(*this, channelIndex, maxBufferSize);
        break;

    default: break;
    }

    return 0;
}

bool
Printer::processChannels()
{
    static Logger::ProcLog log("processChannels", getLog());
    LOGINFO << std::endl;

    // If we are not enabled, just copy the message from the first enabled channel to the output.
    //
    LOGDEBUG << "enabled: " << isEnabled() << "  isIQ: " << isIQ_->getValue() << std::endl;

    // If not printing out, just drop messages onto the floor.
    //
    if (!isEnabled()) {
        for (size_t index = 0; index < getChannelCount(); ++index) getGenericChannelBuffer(index)->popFront();
        return true;
    }

    printHeader(getGenericChannelBuffer(0)->getFront());

    int start = startBin_->getValue();
    int span = count_->getValue();
    for (size_t index = 0; index < getChannelCount(); ++index) {
        Messages::PRIMessage::Ref msg(getGenericChannelBuffer(index)->popFront());
        printers_[index]->print(msg, start, span);
    }

    return true;
}

void
Printer::printHeader(const Messages::PRIMessage::Ref& msg)
{
    std::cout << "Seq: " << msg->getRIUInfo().sequenceCounter << " MsgDesc: " << msg->getRIUInfo().msgDesc
              << " When: " << msg->getRIUInfo().timeStamp << " Shaft: " << msg->getRIUInfo().shaftEncoding
              << " PRF: " << msg->getRIUInfo().prfEncoding << " IRIG: " << msg->getRIUInfo().irigTime
              << " Range: " << msg->getRIUInfo().rangeMin << '/' << msg->getRIUInfo().rangeFactor << '\n';
}

void
Printer::printChannelName(const std::string& name)
{
    static const int maxNameWidth = 16;
    std::cout << "Channel: " << std::setw(maxNameWidth) << name.substr(0, maxNameWidth) << ' ';
}

void
Printer::printVideo(const std::string& channelName, const Messages::Video::Ref& msg, int start, int count)
{
    printChannelName(channelName);
    if (start < msg->size()) {
        if (start + count > msg->size()) count = msg->size() - start;
        Messages::Video::const_iterator pos = msg->begin() + start;
        Messages::Video::const_iterator end = pos + count;
        while (pos != end) std::cout << std::setw(7) << *pos++;
    }

    std::cout << '\n';
}

void
Printer::printBinaryVideo(const std::string& channelName, const Messages::BinaryVideo::Ref& msg, int start, int count)
{
    printChannelName(channelName);
    if (start < msg->size()) {
        if (start + count > msg->size()) count = msg->size() - start;
        Messages::BinaryVideo::const_iterator pos = msg->begin() + start;
        Messages::BinaryVideo::const_iterator end = pos + count;
        while (pos != end) std::cout << std::setw(2) << (*pos++ ? 'T' : '-');
    }

    std::cout << '\n';
}

// Factory function for the DLL that will create a new instance of the Printer class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
PrinterMake(Controller& controller, Logger::Log& log)
{
    return new Printer(controller, log);
}
