#include <cmath>

#include "GUI/Utils.h"

#include "InfoFormatter.h"
#include "ControllerItem.h"
#include "RunnerItem.h"
#include "StreamItem.h"

using namespace SideCar::Algorithms;
using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

ControllerItem::ControllerItem(const ControllerStatus& status,
                               StreamItem* parent)
    : Super(status, parent),
      infoFormatter_(InfoFormatter::Find(
                         QString::fromStdString(
                             getStatus().getAlgorithmName())))
{
    ;
}

ControllerItem::~ControllerItem()
{
    if (infoFormatter_)
	infoFormatter_->release();
}

QVariant
ControllerItem::getNameDataValue(int role) const
{
    return QVariant();
}

QVariant
ControllerItem::getRecordingDataValue(int role) const
{
    return GetRecordingDataValue(getStatus().isRecordingEnabled(), role);
}

QVariant
ControllerItem::getRateDataValue(int role) const
{
    if (role != Qt::DisplayRole)
	return Super::getRateDataValue(role);

    if (! isUsingData())
	return "---";

    double processingTime = getStatus().getAverageProcessingTime();
    double budget = 0.0;
    int messageRate = getMessageRate();
    int percentage = 0;
    if (messageRate > 0) {
	budget = 1.0 / messageRate;
	percentage = ::rint(processingTime / budget * 100);
    }

    QString output("0% of 0");
    if (budget)
	output = QString("%2% of %3")
	    .arg(percentage)
	    .arg(getFormattedProcessingTime(budget));

    output += QString(" (%1/%2/%3)")
	.arg(getFormattedProcessingTime(
                 getStatus().getMinimumProcessingTime()))
	.arg(getFormattedProcessingTime(processingTime))
	.arg(getFormattedProcessingTime(
                 getStatus().getMaximumProcessingTime()));

    return output;
}

QString
ControllerItem::getFormattedProcessingTime(double time) const
{
    if (time == 0.0)
	return "0.0";
    if (time < 1E-6)
	return QString("%1 us").arg(time * 1E9, 0, 'f', 0);
    if (time < 1E-3)
	return QString("%1 ns").arg(time * 1E6, 0, 'f', 0);
    if (time < 1.0)
	return QString("%1 ms").arg(time * 1E3, 0, 'f', 1);
    return QString("%1s").arg(time, 0, 'f', 1);
}

QVariant
ControllerItem::getPendingCountValue(int role) const
{
    if (role != Qt::DisplayRole || ! isRecording())
	return Super::getPendingCountValue(role);
    return QString("%1 (%2)")
	.arg(getPendingQueueCount())
	.arg(getRecordingQueueCount());
}

QVariant
ControllerItem::getInfoDataValue(int role) const
{
    QVariant value = infoFormatter_->format(getStatus(), role);
    if (value.isValid())
	return value;
    return Super::getInfoDataValue(role);
}

void
ControllerItem::afterUpdate()
{
    Super::afterUpdate();
    QString other(QString::fromStdString(getStatus().getAlgorithmName()));
    if (! infoFormatter_ || infoFormatter_->getName() != other)
	infoFormatter_ = InfoFormatter::Find(other);
}

bool
ControllerItem::getParameters(XmlRpc::XmlRpcValue& definition) const
{
    return getParent()->getParameters(getIndex(), definition);
}

bool
ControllerItem::setParameters(const XmlRpc::XmlRpcValue& updates) const
{
    return getParent()->setParameters(getIndex(), updates);
}

void
ControllerItem::fillCollectionStats(CollectionStats& stats) const
{
    Super::fillCollectionStats(stats);

    // Don't count algorithm drops/dupes in the total. stats[CollectionStats::kDropCount] = 0; stats[
    // CollectionStats::kDupeCount] = 0;

    stats[CollectionStats::kCanRecordCount] = 1;
    stats[CollectionStats::kWillRecordCount] = willRecord();
    stats[CollectionStats::kIsRecordingCount] = isRecording();
    stats[CollectionStats::kRecordingQueueCount] = getRecordingQueueCount();
}
