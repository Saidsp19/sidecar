#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "Router.h"
#include "Router_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

Router::Router(Controller& controller, Logger::Log& log)
    : Super(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
      inputChannel_(ChannelParameter::Make("inputChannel", "Input Channel")),
      inputChannelIndex_(kDefaultInputChannel),
      outputChannel_(ChannelParameter::Make("outputChannel", "Output Channel")),
      outputChannelIndex_(kDefaultOutputChannel)
{
    ;
}

bool
Router::startup()
{
    Logger::ProcLog log("startup", getLog());
    LOGINFO << std::endl;

    if (getController().getNumInputChannels() == 0) {
	LOGERROR << "no input channels defined" << std::endl;
	return false;
    }

    if (getController().getNumOutputChannels() == 0) {
	LOGERROR << "no output channels defined" << std::endl;
	return false;
    }

    for (size_t index = 0; index < getController().getNumInputChannels(); ++index) {
	const IO::Channel& channel(getController().getInputChannel(index));
	LOGERROR << "input channel " << index << " name: " << channel.getName() << std::endl;
	inputChannel_->addEnumLabel(channel.getName());
	registerProcessor<Router>(index, *channel.getMetaTypeInfo(), &Router::processInput);
    }

    inputChannel_->setMinValue(Channel(0));
    inputChannel_->setMaxValue(Channel(getController().getNumInputChannels() - 1));
    inputChannel_->setValue(Channel(kDefaultInputChannel));

    for (size_t index = 0; index < getController().getNumOutputChannels(); ++index) {
	const IO::Channel& channel(getController().getOutputChannel(index));
	LOGERROR << "output channel " << index << " name: " << channel.getName() << std::endl;
	outputChannel_->addEnumLabel(channel.getName());
    }

    outputChannel_->setMinValue(Channel(0));
    outputChannel_->setMaxValue(Channel(getController().getNumOutputChannels() - 1));
    outputChannel_->setValue(Channel(kDefaultOutputChannel));

    LOGERROR << "initialized output channel parameter" << std::endl;

    return registerParameter(enabled_) && registerParameter(inputChannel_) && registerParameter(outputChannel_) &&
        Super::startup();
}

bool
Router::shutdown()
{
    return Super::shutdown();
}

bool
Router::processInput(const Messages::Header::Ref& msg)
{
    if (getActiveChannelIndex() == inputChannelIndex_)
	return send(msg, outputChannelIndex_);
    return true;
}

void
Router::endParameterChanges()
{
    inputChannelIndex_ = inputChannel_->getValue();
    outputChannelIndex_ = outputChannel_->getValue();
}

void
Router::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
    status.setSlot(kInputChannel, inputChannel_->getEnumLabel(inputChannel_->getValue()));
    status.setSlot(kOutputChannel, outputChannel_->getEnumLabel(outputChannel_->getValue()));
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[Router::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    std::string inputChannel = status[Router::kInputChannel];
    std::string outputChannel = status[Router::kOutputChannel];
    return Algorithm::FormatInfoValue(QString("Input: %1  Output: %2")
                                      .arg(QString::fromStdString(inputChannel))
                                      .arg(QString::fromStdString(outputChannel)));
}

// Factory function for the DLL that will create a new instance of the Router class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
RouterMake(Controller& controller, Logger::Log& log)
{
    return new Router(controller, log);
}
