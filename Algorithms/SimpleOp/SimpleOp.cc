#include "Logger/Log.h"

#include "SimpleOp.h"
#include "SimpleOp_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

static const char* kOperatorNames[] = {
    "Sum",
    "Difference",
    "Product",
    "Min",
    "Max"
};

const char* const*
SimpleOp::OperatorEnumTraits::GetEnumNames()
{
    return kOperatorNames;
}

SimpleOp::SimpleOp(Controller& controller, Logger::Log& log)
    : Super(controller, log, kDefaultEnabled, kDefaultMaxBufferSize),
      operator_(OperatorParameter::Make("operator", "Operator", Operator(kDefaultOperator)))
{
    ;
}

bool
SimpleOp::startup()
{
    return Super::startup() && registerParameter(operator_);
} 

bool
SimpleOp::processChannels()
{
    static Logger::ProcLog log("processChannels", getLog());
    LOGINFO << std::endl;

    // If we are not enabled, just copy the message from the first enabled channel to the output.
    //
    if (! isEnabled()) {
	LOGDEBUG << "disabled" << std::endl;
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
	    if (! out) {
		out = Messages::Video::Make(getName(), msg);
		size = msg->size();
	    }
	    else if (size > msg->size()) {
		size = msg->size();
	    }
	    iterators.push_back(msg->begin());
	}
    }

    if (! out) {
	LOGWARNING << "enabled but no channels enabled" << std::endl;
	return true;
    }

    Operator op = operator_->getValue();
    LOGDEBUG << "operation " << op << ' ' << kOperatorNames[op] << std::endl;

    while (size-- != 0) {
	IteratorVector::iterator pos = iterators.begin();
	Messages::Video::DatumType value = *(*pos++)++;
	IteratorVector::iterator end = iterators.end();
	switch (op) {
	case kSumOp:	// A + B
	    while (pos != end) value += *(*pos++)++;
	    break;

	case kDiffOp:	// A - B
	    while (pos != end) value -= *(*pos++)++;
	    break;

	case kProdOp:	// A * B
	    while (pos != end) value *= *(*pos++)++;
	    break;

	case kMinOp:	// MIN(A, B)
	    while (pos != end) value = std::min(value, *(*pos++)++);
	    break;

	case kMaxOp:	// MAX(A, B)
	    while (pos != end) value = std::max(value, *(*pos++)++);
	    break;

	default: break;
	}

	out->push_back(value);
    }

    LOGDEBUG << out->headerPrinter() << std::endl;
    LOGDEBUG << out->dataPrinter() << std::endl;

    // Remove the message from the buffers.
    //
    for (size_t index = 0; index < getChannelCount(); ++index) {
	VideoChannelBuffer* channel = getChannelBuffer<Messages::Video>(index);
	if (channel->isEnabled()) {
	    Messages::Video::Ref msg(channel->popFront());
	}
    }

    return send(out);
}

ChannelBuffer*
SimpleOp::makeChannelBuffer(int channelIndex, const std::string& name, size_t maxBufferSize)
{
    VideoChannelBuffer* channel = new VideoChannelBuffer(*this, channelIndex, maxBufferSize);
    channel->makeEnabledParameter();
    return channel;
}

void
SimpleOp::setInfoSlots(IO::StatusBase& status)
{
    Super::setInfoSlots(status);
    status.setSlot(kOperator, operator_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[ManyInAlgorithm::kEnabled])
	return Algorithm::FormatInfoValue(ManyInAlgorithm::GetFormattedStats(status));

    return Algorithm::FormatInfoValue(ManyInAlgorithm::GetFormattedStats(status) +
                                      QString(kOperatorNames[int(status[SimpleOp::kOperator])]));
}

// Factory function for the DLL that will create a new instance of the SimpleOp class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
SimpleOpMake(Controller& controller, Logger::Log& log)
{
    return new SimpleOp(controller, log);
}
