#include "boost/bind.hpp"

#include "IO/MessageManager.h"
#include "Logger/Log.h"

#include "SandBox.h"

using namespace SideCar::Algorithms;

SandBox::SandBox(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), intValue_(Parameter::IntValue::Make("intValue", "Int Value", 0)),
    boolValue_(Parameter::BoolValue::Make("boolValue", "Bool Value", false)),
    doubleRange_(DRange::Make("doubleRange", "Double Range Value", 0.5)),
    pathValue_(
        Parameter::ReadPathValue::Make("pathValue", "Path Value", "/opt/sidecar/builds/latest/data/configuration.xml")),
    notificationValue_(Parameter::NotificationValue::Make("notificationValue", "Notification Value", 0))
{
    intValue_->connectChangedSignalTo(boost::bind(&SandBox::intValueChanged, this, _1));
    boolValue_->connectChangedSignalTo(boost::bind(&SandBox::boolValueChanged, this, _1));
    pathValue_->connectChangedSignalTo(boost::bind(&SandBox::pathValueChanged, this, _1));
    notificationValue_->connectChangedSignalTo(boost::bind(&SandBox::notificationValueChanged, this, _1));
}

bool
SandBox::startup()
{
    registerProcessor<SandBox, Messages::Video>("one", &SandBox::processOne);
    registerProcessor<SandBox, Messages::Video>("two", &SandBox::processTwo);
    registerProcessor<SandBox, Messages::Video>("three", &SandBox::processThree);
    registerProcessor<SandBox, Messages::Video>("four", &SandBox::processFour);

    oneChannelIndex_ = getOutputChannelIndex("one");
    twoChannelIndex_ = getOutputChannelIndex("two");
    threeChannelIndex_ = getOutputChannelIndex("three");
    fourChannelIndex_ = getOutputChannelIndex("four");

    return registerParameter(intValue_) && registerParameter(boolValue_) && registerParameter(doubleRange_) &&
           registerParameter(pathValue_) && registerParameter(notificationValue_) && Algorithm::startup();
}

bool
SandBox::processOne(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processOne", getLog());
    LOGINFO << std::endl;
    Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
    out->getData() = msg->getData();
    bool rc = send(out, oneChannelIndex_);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

bool
SandBox::processTwo(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processTwo", getLog());
    LOGINFO << std::endl;
    Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
    out->getData() = msg->getData();
    bool rc = send(out, twoChannelIndex_);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

bool
SandBox::processThree(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processThree", getLog());
    LOGINFO << std::endl;
    Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
    out->getData() = msg->getData();
    bool rc = send(out, threeChannelIndex_);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

bool
SandBox::processFour(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processFour", getLog());
    LOGINFO << std::endl;
    Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
    out->getData() = msg->getData();
    bool rc = send(out, fourChannelIndex_);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
SandBox::intValueChanged(const Parameter::IntValue&)
{
    Logger::ProcLog log("intValueChanged", getLog());
    LOGERROR << "value: " << intValue_->getValue() << std::endl;
}

void
SandBox::boolValueChanged(const Parameter::BoolValue&)
{
    Logger::ProcLog log("boolValueChanged", getLog());
    LOGERROR << "value: " << boolValue_->getValue() << std::endl;
}

void
SandBox::pathValueChanged(const Parameter::ReadPathValue&)
{
    Logger::ProcLog log("pathValueChanged", getLog());
    LOGERROR << "value: " << pathValue_->getValue() << std::endl;
}

void
SandBox::notificationValueChanged(const Parameter::NotificationValue&)
{
    Logger::ProcLog log("notificationValueChanged", getLog());
    LOGERROR << "value: " << notificationValue_->getValue() << std::endl;
}

/** Declare/define an entry point for the SandBox DLL which allows a Controller object to create a new SandBox
    instance to manage.

    \param controller the Controller that will manage the SandBox object

    \param log the log device to use for SandBox log messages.

    \return
*/
extern "C" ACE_Svc_Export Algorithm*
SandBoxMake(Controller& controller, Logger::Log& log)
{
    return new SandBox(controller, log);
}
