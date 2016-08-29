#include <cmath>
#include "boost/bind.hpp"

#include "Runner/RemoteController.h"
#include "Runner/RunnerStatus.h"
#include "GUI/ServiceEntry.h"
#include "GUI/LogUtils.h"
#include "IO/ProcessingState.h"
#include "XMLRPC/XmlRpcValue.h"

#include "App.h"
#include "ConfigurationController.h"
#include "ConfigurationItem.h"
#include "MainWindow.h"
#include "RootItem.h"
#include "RunnerItem.h"
#include "ServicesModel.h"
#include "StatusCollector.h"
#include "StreamItem.h"

#include "QtCore/QMetaType"
#include "QtCore/QVariant"
#include "QtGui/QMessageBox"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

const char* ServicesModel::kColumnNames_[] = {
    "Name",
    "Host",
    "State",
    "Rec",
    "Pending",
    "Activity",
    "Error",
    "Info"
};

Logger::Log&
ServicesModel::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.ServicesModel");
    return log_;
}

TreeViewItem*
ServicesModel::GetModelData(const QModelIndex& index)
{
    static Logger::ProcLog log("GetModelData", Log());
    LOGINFO << "row: " << index.row() << " col: " << index.column() << " p: "
            << index.internalPointer() << " rows: "
            << index.model()->rowCount() << " cols: "
            << index.model()->columnCount() << std::endl;
    if (! index.isValid()) return 0;
    return static_cast<TreeViewItem*>(index.internalPointer());
}

QString
ServicesModel::GetColumnName(int index)
{
    return QString(kColumnNames_[index]);
}

ServicesModel::ServicesModel(QObject* parent)
    : Super(parent),
      browser_(new ServiceBrowser(
                   this, QString::fromStdString(
                       Runner::RemoteController::GetZeroconfType()))),
      statusCollector_(new StatusCollector), rootItem_(new RootItem)
{
    static Logger::ProcLog log("ServicesModel", Log());
    connect(browser_, SIGNAL(foundServices(const ServiceEntryList&)),
            SLOT(foundServices(const ServiceEntryList&)));
    connect(browser_, SIGNAL(lostServices(const ServiceEntryList&)),
            SLOT(lostServices(const ServiceEntryList&)));
    connect(statusCollector_,
            SIGNAL(statusUpdates(const QList<QByteArray>&)),
            SLOT(updateStatus(const QList<QByteArray>&)));
}

ServicesModel::~ServicesModel()
{
    ;
}

bool
ServicesModel::start()
{
    browser_->start();
    return statusCollector_->open();
}

QModelIndex
ServicesModel::getModelIndex(const TreeViewItem* item, int col) const
{
    QModelIndex parentIndex;
    if (item->getParent())
	parentIndex = getModelIndex(item->getParent(), 0);
    return index(item->getIndex(), col, parentIndex);
}

int
ServicesModel::columnCount(const QModelIndex& parent) const
{
    return kNumColumns;
}

int
ServicesModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
	return 0;
    if (! parent.isValid()) return rootItem_->getNumChildren();
    return GetModelData(parent)->getNumChildren();
}

QModelIndex
ServicesModel::index(int row, int column, const QModelIndex& parent) const
{
    QModelIndex index;
    if (hasIndex(row, column, parent)) {
	TreeViewItem* pobj = parent.isValid() ? GetModelData(parent) :
	    rootItem_;
	if (row >= 0 && row < pobj->getNumChildren())
	    index = createIndex(row, column, pobj->getChild(row));
    }

    return index;
}

QModelIndex
ServicesModel::parent(const QModelIndex& index) const
{
    if (! index.isValid()) return QModelIndex();
    TreeViewItem* obj = GetModelData(index);
    if (! obj) return QModelIndex();
    obj = obj->getParent();
    if (! obj || obj == rootItem_) return QModelIndex();
    return createIndex(obj->getIndex(), 0, obj);
}

bool
ServicesModel::postProcessingStateChange(
    const QStringList& filter, IO::ProcessingState::Value state) const
{
    Logger::ProcLog log("postProcessingStateChange", Log());
    LOGINFO << state << std::endl;
    XmlRpc::XmlRpcValue args;
    args.setSize(0);
    args.push_back(int(state));
    return postCommand(filter, "stateChange", args);
}

bool
ServicesModel::postClearStats(const QStringList& filter) const
{
    Logger::ProcLog log("postClearStats", Log());
    LOGINFO << std::endl;
    XmlRpc::XmlRpcValue args;
    return postCommand(filter, "clearStats", args);
}

bool
ServicesModel::postRecordingStart(const QStringList& configNames,
                                  const QStringList& recordingPaths) const
{
    Logger::ProcLog log("postRecordingStart", Log());
    LOGINFO << std::endl;
    for (int index = 0; index < configNames.count(); ++index) {
	XmlRpc::XmlRpcValue args;
	args.setSize(0);
	args.push_back(recordingPaths[index].toStdString());
	if (! postCommand(configNames[index], "recordingChange", args))
	    return false;
    }	

    return true;
}

bool
ServicesModel::postRecordingStop(const QStringList& configNames) const
{
    Logger::ProcLog log("postRecordingStop", Log());
    LOGINFO << std::endl;
    XmlRpc::XmlRpcValue args;
    args.setSize(0);
    args.push_back("");
    return postCommand(configNames, "recordingChange", args);
}

bool
ServicesModel::postShutdownRequest(const QString& configName) const
{
    Logger::ProcLog log("postShutdownRequest", Log());
    LOGINFO << std::endl;
    XmlRpc::XmlRpcValue args;
    return postCommand(configName, "shutdown", args);
}

bool
ServicesModel::postCommand(const QStringList& filter, const char* cmd,
                           const XmlRpc::XmlRpcValue& args) const
{
    for (int index = 0; index < filter.count(); ++index) {
	if (! postCommand(filter[index], cmd, args))
	    return false;
    }
    return true;
}

bool
ServicesModel::postCommand(const QString& configName, const char* cmd,
                           const XmlRpc::XmlRpcValue& args) const
{
    Logger::ProcLog log("postCommand", Log());
    LOGINFO << "configName: " << configName << " cmd: " << cmd << std::endl;

    for (int index = 0; index < rootItem_->getNumChildren(); ++index) {
	const ConfigurationItem* configItem = rootItem_->getChild(index);
	if (configName == configItem->getName())
	    return configItem->executeRequest(cmd, args);
    }

    return false;
}

bool
ServicesModel::getChangedParameters(const QStringList& configNames,
                                    QStringList& changes) const
{
    bool ok = true;
    foreach (QString configName, configNames) {
	ConfigurationItem* configItem = getConfigurationItem(configName);
	if (configItem) {
	    if (! configItem->getChangedParameters(changes))
		ok = false;
	}
    }
    return ok;
}

void
ServicesModel::foundServices(const ServiceEntryList& found)
{
    static Logger::ProcLog log("foundServices", Log());
    LOGINFO << found.size() << std::endl;

    // Resolve each of the found services before adding them to any existing RunnerItem objects.
    //
    for (ServiceEntryList::const_iterator pos = found.begin();
         pos != found.end(); ++pos) {
	ServiceEntry* serviceEntry(*pos);
	LOGDEBUG << serviceEntry->getName() << std::endl;
	connect(serviceEntry, SIGNAL(resolved(ServiceEntry*)),
                SLOT(resolvedService(ServiceEntry*)));
	serviceEntry->resolve();
    }
}

void
ServicesModel::resolvedService(ServiceEntry* serviceEntry)
{
    static Logger::ProcLog log("resolvedService", Log());
    LOGINFO << "serviceEntry name: " << serviceEntry->getName() << std::endl;

    // Visit each ConfigurationItem and look for an entry that matches the resolved ServiceEntry object. Not an
    // error if not found, since it is possible that we have not yet received any status from the runner
    // represented by the resolved ServiceEntry object.
    //
    for (int index = 0; index < rootItem_->getNumChildren(); ++index) {
	ConfigurationItem* configurationItem = rootItem_->getChild(index);
	RunnerItem* runnerItem =
	    configurationItem->findService(serviceEntry);
	if (runnerItem) {
	    runnerItem->setServiceEntry(serviceEntry);
	    return;
	}
    }
}

void
ServicesModel::lostServices(const ServiceEntryList& lost)
{
    static Logger::ProcLog log("lostServices", Log());
    LOGINFO << "lost.count: " << lost.count() << std::endl;

    for (ServiceEntryList::const_iterator pos = lost.begin();
         pos != lost.end(); ++pos) {

	ServiceEntry* serviceEntry(*pos);
	LOGDEBUG << "lost " << serviceEntry->getName() << std::endl;
	serviceEntry->disconnect(this);

	for (int index = 0; index < rootItem_->getNumChildren(); ++index) {
	    ConfigurationItem* configurationItem = rootItem_->getChild(index);
	    RunnerItem* runnerItem =
		configurationItem->findService(serviceEntry);
	    if (runnerItem) {
		emit runnerRemoved(runnerItem);
		
		if (configurationItem->getNumChildren() == 1) {
		    int row = configurationItem->getIndex();
		    beginRemoveRows(QModelIndex(), row, row);

		    // !!! NOTE: configurationItem and its children (including runnerItem) no longer exist after
		    // this!!!
		    //
		    rootItem_->removeChild(row);
		    endRemoveRows();
		}
		else {
		    int row = runnerItem->getIndex();
		    beginRemoveRows(getModelIndex(configurationItem), row,
                                    row);
		    // !!! NOTE: runnerItem and its children are no longer exist after this!!!
		    //
		    configurationItem->removeChild(row);
		    endRemoveRows();
		}
		break;
	    }
	}
    }

    emit statusUpdated();
}

QVariant
ServicesModel::headerData(int section, Qt::Orientation orientation,
                          int role) const
{
    if (orientation == Qt::Vertical)
	return QVariant();

    if (role == Qt::TextAlignmentRole)
	return int(Qt::AlignVCenter | Qt::AlignHCenter);

    if (role == Qt::DisplayRole)
	return QString(GetColumnName(section));

    return QVariant();
}

QVariant
ServicesModel::data(const QModelIndex& index, int role) const
{
    if (! index.isValid())
	return QVariant();
    return GetModelData(index)->getData(index.column(), role);
}

Qt::ItemFlags
ServicesModel::flags(const QModelIndex& index) const
{
    if (! index.isValid())
	return Super::flags(index);
    Qt::ItemFlags flags = Qt::ItemIsEnabled;
    return flags;
}

void
ServicesModel::updateStatus(const QList<QByteArray>& statusReports)
{
    static Logger::ProcLog log("updateStatus", Log());

    foreach (QByteArray statusReport, statusReports) {

	// Verify that we have a valid XML-RPC value. For some reason I've seen corrupted XML-RPC messages.
	// Still don't know why...
	//
	int offset = 0;
	XmlRpc::XmlRpcValue xmlValue(statusReport.data(), &offset);
	if (! xmlValue.valid() ||
            xmlValue.getType() != XmlRpc::XmlRpcValue::TypeArray) {
	    LOGERROR << "invalid type from runner: " << xmlValue.getType()
		     << " size: " << statusReport.size() << std::endl;
	    continue;
	}

	// Lame attempt at versioning XML-RPC messages. Throw it out?
	//
	Runner::RunnerStatus status(xmlValue);
	if (! status.isLatestVersion()) {
	    LOGWARNING << "skipping status report with version "
		       << status.getVersion() << std::endl;
	    continue;
	}

	QString serviceName(
	    QString::fromStdString(status.getServiceName()));
	LOGDEBUG << "serviceName: " << serviceName << std::endl;

	// See if there is a service entry for the runner. It may be that it quit right after sending a status
	// message, and we are processing a ghost message.
	//
	ServiceEntry* serviceEntry = browser_->getServiceEntry(serviceName);
	if (! serviceEntry)
	    continue;

	// Obtain the configuration item that contains the runner sending us status.
	//
	QString configName(QString::fromStdString(status.getConfigName()));
	LOGDEBUG << "configName: " << configName << std::endl;
	ConfigurationItem* configurationItem = rootItem_->find(configName);
	if (! configurationItem) {

	    // Does not exist. Create an object to represent the configuration and update the view.
	    //
	    configurationItem = new ConfigurationItem(status, rootItem_);
	    int row = rootItem_->getInsertionPoint(configName);
	    beginInsertRows(QModelIndex(), row, row);
	    rootItem_->insertChild(row, configurationItem);
	    endInsertRows();
	}

	// Locate the runner item that represents the runner sending us status.
	//
	RunnerItem* runnerItem = configurationItem->findService(serviceName);
	if (! runnerItem) {

	    // Does not exist. Create an object to represent the runner and update the view.
	    //
	    runnerItem = new RunnerItem(status, configurationItem);

	    int row = configurationItem->getInsertionPoint(
		runnerItem->getName());

	    LOGDEBUG << "beginInsertRows row: " << row << ' ' << serviceName
		     << std::endl;
	    beginInsertRows(getModelIndex(configurationItem), row, row);
	    configurationItem->insertChild(row, runnerItem);
	    endInsertRows();

	    emit runnerAdded(runnerItem);
	    emit statusUpdated();
	}
	else {

	    // Just update the status of the existing runner and if necessary, update the status display.
	    //
	    if (configurationItem->update(status, runnerItem)) {
		emit dataChanged(getModelIndex(runnerItem, kState),
                                 getModelIndex(runnerItem,
                                               kNumColumns - 1));
		emit statusUpdated();
	    }
	}

	// If we don't have a service entry for the object, try and set one.
	//
	if (! runnerItem->getServiceEntry()) {
	    ServiceEntry* serviceEntry = browser_->getServiceEntry(
		serviceName);
	    LOGDEBUG << "serviceName: " << serviceName << " serviceEntry: "
		     << serviceEntry << std::endl;
	    if (serviceEntry) {
		if (serviceEntry->isResolved()) {
		    LOGDEBUG << "resolved" << std::endl;
		    runnerItem->setServiceEntry(serviceEntry);
		}
	    }
	}
    }
}

int
ServicesModel::getConfigurationCount() const
{
    return rootItem_->getNumChildren();
}

void
ServicesModel::getServiceStats(int& runnerCount, int& streamCount,
                               int& pendingCount, int& failureCount) const
{
    runnerCount = 0;
    streamCount = 0;
    pendingCount = 0;
    failureCount = 0;
    for (int index = 0; index < rootItem_->getNumChildren(); ++index) {
	ConfigurationItem* configItem = rootItem_->getChild(index);
	runnerCount += configItem->getNumChildren();
	streamCount += configItem->getStreamCount();
	const CollectionStats& stats(configItem->getCollectionStats());
	pendingCount += stats.getPendingQueueCount();
	failureCount += stats.getFailureCount();
    }
}

void
ServicesModel::getDropsAndDupes(const QStringList& filter, int& drops,
                                int& dupes) const
{
    drops = 0;
    dupes = 0;
    for (int index = 0; index < rootItem_->getNumChildren(); ++index) {
	ConfigurationItem* configItem = rootItem_->getChild(index);
	if (filter.contains(configItem->getName())) {
	    const CollectionStats& stats(configItem->getCollectionStats());
	    drops += stats.getDropCount();
	    dupes += stats.getDupeCount();
	}
    }
}

RunnerItem*
ServicesModel::getRunnerItem(const QString& configName,
                             const QString& runnerName) const
{
    for (int index = 0; index < rootItem_->getNumChildren(); ++index) {
	ConfigurationItem* configItem = rootItem_->getChild(index);
	if (configItem->getName() == configName) {
	    RunnerItem* runnerItem = configItem->findService(runnerName);
	    if (runnerItem)
		return runnerItem;
	    break;
	}
    }

    return 0;
}

QStringList
ServicesModel::getServiceNames(const QString& configName) const
{
    QStringList names;
    ConfigurationItem* configItem = rootItem_->find(configName);
    if (configItem) {
	for (int index = 0; index < configItem->getNumChildren();
             ++index) {
	    RunnerItem* runnerItem = configItem->getChild(index);
	    names.append(runnerItem->getServiceName());
	}
    }

    return names;
}

bool
ServicesModel::isRecording(const QStringList& filter) const
{
    for (int index = 0; index < rootItem_->getNumChildren(); ++index) {
	ConfigurationItem* configItem = rootItem_->getChild(index);
	if (filter.contains(configItem->getName()) &&
            configItem->isRecording()) return true;
    }

    return false;
}

bool
ServicesModel::isCalibrating(const QStringList& filter) const
{
    for (int index = 0; index < rootItem_->getNumChildren(); ++index) {
	ConfigurationItem* configItem = rootItem_->getChild(index);
	if (filter.contains(configItem->getName()) &&
            configItem->getProcessingState() ==
            IO::ProcessingState::kCalibrate) return true;
    }

    return false;
}

ConfigurationItem*
ServicesModel::getConfigurationItem(const QString& name) const
{
    return rootItem_->find(name);
}
