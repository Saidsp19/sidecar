#include "boost/bind.hpp"

#include "Algorithms/Controller.h"
#include "Algorithms/Utils.h"
#include "Logger/Log.h"

#include "BugCollector.h"
#include "BugPlotSubscriber.h"
#include "BugCollector_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Algorithms::BugCollectorUtils;

BugCollector::BugCollector(Controller& controller, Logger::Log& log)
    : Super(controller, log), subscriber_(),
      prefix_(Parameter::StringValue::Make("prefix", "User Bug Channel Prefix", "UserBugs"))
{
    prefix_->connectChangedSignalTo(boost::bind(&BugCollector::prefixChanged, this, _1));
}

bool
BugCollector::startup()
{
    subscriber_.reset(new BugPlotSubscriber(*this, prefix_->getValue()));
    return Super::startup() && registerParameter(prefix_);
}

void
BugCollector::process(const Messages::BugPlot::Ref& msg)
{
    static Logger::ProcLog log("process", getLog());
    LOGINFO << std::endl;
    getController().updateInputStats(0, msg->getSize(), msg->getMessageSequenceNumber());
    send(msg);
}

void
BugCollector::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kPrefix, prefix_->getValue());
    status.setSlot(kActive, static_cast<int>(subscriber_->size()));
}

void
BugCollector::prefixChanged(const Parameter::StringValue& parameter)
{
    subscriber_->setPrefix(parameter.getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    QString prefix = QString::fromStdString(status[BugCollector::kPrefix]);
    int size = status[BugCollector::kActive];
    return Algorithm::FormatInfoValue(QString("Prefix: %1  Subscriptions: %2").arg(prefix).arg(size));
}

extern "C" ACE_Svc_Export Algorithm*
BugCollectorMake(Controller& controller, Logger::Log& log)
{
    return new BugCollector(controller, log);
}
