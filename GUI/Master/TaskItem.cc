#include <cmath>
#include <iostream>

#include "QtGui/QColor"

#include "GUI/LogUtils.h"

#include "StreamItem.h"
#include "ServicesModel.h"
#include "TaskItem.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

Logger::Log&
TaskItem::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.TaskItem");
    return log_;
}

TaskItem::TaskItem(const IO::TaskStatus& status, StreamItem* parent)
    : Super(status, parent), lastMessageCount_(0),
      error_(QString::fromStdString(getStatus().getError())),
      connectionInfo_(
	  QString::fromStdString(getStatus().getConnectionInfo()))
{
    ;
}

QVariant
TaskItem::getNameDataValue(int role) const
{
    if (role == Qt::ToolTipRole && ! connectionInfo_.isEmpty())
	return connectionInfo_;
    return Super::getNameDataValue(role);
}

QVariant
TaskItem::getStateDataValue(int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    return IO::ProcessingState::GetName(getStatus().getProcessingState());
}

QVariant
TaskItem::getPendingCountValue(int role) const
{
    if (role != Qt::DisplayRole) return Super::getPendingCountValue(role);
    if (! isUsingData())
	return "---";
    return getPendingQueueCount();
}

QVariant
TaskItem::getRateDataValue(int role) const
{
    static int const kKB = 1000;
    static int const kMB = kKB * 1000;

    int dropCount = getDropCount();
    int dupeCount = getDupeCount();
    if (role == Qt::ForegroundRole)
	return (dropCount || dupeCount) ?
	    GetFailureColor() : Super::getRateDataValue(role);

    if (role != Qt::DisplayRole)
	return Super::getRateDataValue(role);

    if (! isUsingData())
	return QString("---");

    QString output("%1 (%2 m/s - %3 %4/s)");
    output = output.arg(getMessageCount()).arg(getMessageRate());

    int rate = getByteRate();
    if (rate > kMB) {
	output = output.arg(::round(rate / kMB)).arg("MB");
    }
    else if (rate > kKB) {
	output = output.arg(::round(rate / kKB)).arg("KB");
    }
    else {
	output = output.arg(::round(rate)).arg("B");
    }

    if (dropCount)
	output += QString(" Drops: %1").arg(dropCount);

    if (dupeCount)
	output += QString(" Dupes: %1").arg(dupeCount);

    return output;
}

QVariant
TaskItem::getErrorDataValue(int role) const
{
    if (role == Qt::DisplayRole) return error_;
    return Super::getErrorDataValue(role);
}

QVariant
TaskItem::getInfoDataValue(int role) const
{
    if (role == Qt::DisplayRole) return connectionInfo_;
    return Super::getInfoDataValue(role);
}

StreamItem*
TaskItem::getParent() const
{
    return static_cast<StreamItem*>(Super::getParent());
}

void
TaskItem::beforeUpdate()
{
    lastMessageCount_ = getStatus().getMessageCount();
}

void
TaskItem::afterUpdate()
{
    static Logger::ProcLog log("afterUpdate", Log());
    Super::afterUpdate();
    error_ = QString::fromStdString(getStatus().getError());
    connectionInfo_ = QString::fromStdString(getStatus().getConnectionInfo());
    IO::ProcessingState::Value state = getStatus().getProcessingState();

    setOK(error_.isEmpty());
    setProcessingState(state);

    if (! isUsingData())
	setActiveState(kNotUsingData);
    else
	setActiveState(lastMessageCount_ == getStatus().getMessageCount() ?
                       kIdle : kActive);
}

void
TaskItem::updateCollectionStats(CollectionStats& stats) const
{
    CollectionStats us;
    fillCollectionStats(us);
    stats += us;
}

void
TaskItem::fillCollectionStats(CollectionStats& stats) const
{
    stats[CollectionStats::kLeafCount] = 1;
    stats[CollectionStats::kOKCount] = isOK() ? 1 : 0;

    switch (getActiveState()) {
    case kActive: stats[CollectionStats::kActiveCount] = 1; break;
    case kIdle: stats[CollectionStats::kIdleCount] = 1; break;
    case kNotUsingData: stats[CollectionStats::kNotUsingDataCount] = 1; break;
    default: break;
    }

    stats[CollectionStats::kPendingQueueCount] = getPendingQueueCount();
    stats[CollectionStats::kDropCount] = getDropCount();
    stats[CollectionStats::kDupeCount] = getDupeCount();

    stats.setProcessingState(getProcessingState());
    stats.setError(getError());
}

void
TaskItem::formatChangedParameters(const XmlRpc::XmlRpcValue& definitions,
                                  QStringList& changes) const
{
    // Create a list of strings that detail the paramter changes found in the given XML array.
    //
    QString heading;
    for (int index = 0; index < definitions.size(); ++index) {

	// Fetch the XML struct that defines the parameter that has changed
	//
	const XmlRpc::XmlRpcValue& definition(definitions[index]);
	QString value, original;

	// Format according to parameter type.
	//
	std::string type(definition["type"]);
	if (type == "int") {
	    value = QString::number(int(definition["value"]));
	    original = QString::number(int(definition["original"]));
	}
	else if (type == "double") {
	    value = QString::number(double(definition["value"]));
	    original = QString::number(double(definition["original"]));
	}
	else if (type == "bool") {
	    value = bool(definition["value"]) ? "TRUE" : "FALSE";
	    original = bool(definition["original"]) ? "TRUE" : "FALSE";
	}
	else if (type == "enum") {

	    // Convert enumerated values into text strings. Make sure that the enumeration values are valid.
	    //
	    const XmlRpc::XmlRpcValue::ValueArray& names(
		definition["enumNames"]);
	    int min = int(definition["min"]);

	    // Get string of the current value
	    //
	    int tmp = int(definition["value"]) - min;
	    if (tmp >= 0 && tmp < names.size())
		value = QString("'%1'").arg(
		    QString::fromStdString(names[tmp]));
	    
	    // Get string of the original value
	    //
	    tmp = int(definition["original"]) - min;
	    if (tmp >= 0 && tmp < names.size())
		original = QString("'%1'").arg(
		    QString::fromStdString(names[tmp]));
	}
	else if (type == "notification") {
	    continue;
	}
	else {			// string, readPath, and writePath
	    value = QString("'%1'").arg(
		QString::fromStdString(definition["value"]));
	    original = QString("'%1'").arg(
		QString::fromStdString(definition["original"]));
	}

	if (heading.isNull()) {
	    heading = getParameterChangedHeading();
	    changes.append(heading);
	}

	// Format an entry for logging
	//
	changes.append(QString("  Param '%1' = %2  [original is %3]\n")
                       .arg(QString::fromStdString(definition["name"]))
                       .arg(value)
                       .arg(original));
    }
}

QString
TaskItem::getParameterChangedHeading() const
{
    TreeViewItem* streamItem = getParent();
    TreeViewItem* runnerItem = streamItem->getParent();
    TreeViewItem* configItem = runnerItem->getParent();
    return QString("* Path: '%1' -> '%2' -> '%3' -> '%4'\n")
	.arg(configItem->getName())
	.arg(runnerItem->getName())
	.arg(streamItem->getName())
	.arg(getName());
}
