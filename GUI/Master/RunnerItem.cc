#include "GUI/LogUtils.h"
#include "GUI/ServiceEntry.h"
#include "GUI/Utils.h"
#include "XMLRPC/XmlRpcClient.h"
#include "XMLRPC/XmlRpcValue.h"

#include "ConfigurationItem.h"
#include "RootItem.h"
#include "RunnerItem.h"
#include "RunnerLog.h"
#include "StreamItem.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;
using namespace SideCar::Runner;

Logger::Log&
RunnerItem::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.RunnerItem");
    return log_;
}

RunnerItem::RunnerItem(const RunnerStatus& status, ConfigurationItem* parent) :
    Super(status, parent), serviceEntry_(0), log_(0)
{
    Logger::ProcLog log("RunnerItem", Log());
    LOGINFO << std::endl;

    int streamCount = status.getStreamCount();
    LOGDEBUG << "streamCount: " << streamCount << std::endl;
    for (int index = 0; index < streamCount; ++index) {
        StreamItem* child = new StreamItem(getStatus().getStreamStatus(index), this);
        int row = getInsertionPoint(child->getName());
        LOGDEBUG << "row: " << row << std::endl;
        insertChild(row, child);
        for (size_t mapIndex = 0; mapIndex < streamIndexMapping_.size(); ++mapIndex) {
            if (streamIndexMapping_[mapIndex] >= row) ++streamIndexMapping_[mapIndex];
        }
        streamIndexMapping_.push_back(row);
    }
}

void
RunnerItem::updateChildren()
{
    Logger::ProcLog log("updateChildren", Log());
    LOGINFO << "self: " << getIndex() << std::endl;
    for (int index = 0; index < getNumChildren(); ++index) {
        int row = streamIndexMapping_[index];
        LOGDEBUG << "child index: " << index << " row: " << row << std::endl;
        StreamItem* child = getChild(row);
        child->update(getStatus().getStreamStatus(index));
    }
}

QVariant
RunnerItem::getHostDataValue(int role) const
{
    if (role == Qt::ToolTipRole) {
        return QString("Port %1").arg(serviceEntry_ ? serviceEntry_->getPort() : -1);
    } else if (role == Qt::DisplayRole) {
        return getHostName();
    }
    return Super::getHostDataValue(role);
}

QVariant
RunnerItem::getInfoDataValue(int role) const
{
    double memoryUsed = getStatus().getMemoryUsed();
    if (role == Qt::ForegroundRole) {
        static double warning = 500.0 * 1024 * 1024; // 500M
        static double danger = 1000.0 * 1024 * 1024; // 1G
        if (memoryUsed >= danger) return GetFailureColor();
        if (memoryUsed >= warning) return GetWarningColor();
        return GetOKColor();
    }

    if (role == Qt::DisplayRole) return QString("Mem: ") + ByteAmountToString(memoryUsed, 1);

    return Super::getInfoDataValue(role);
}

bool
RunnerItem::executeRequest(const char* cmd, const XmlRpc::XmlRpcValue& args, XmlRpc::XmlRpcValue& result) const
{
    static Logger::ProcLog log("executeRequest", Log());
    LOGINFO << "cmd: " << cmd << std::endl;

    if (!serviceEntry_) {
        LOGERROR << "unresolved runner - service: " << getServiceName() << " host: " << getHostName()
                 << std::endl;
        return true;
    }

    LOGINFO << "cmd: " << cmd << " host: " << getHostName() << " port: " << serviceEntry_->getPort() << std::endl;

    XmlRpc::XmlRpcClient client(getHostName().toStdString(), serviceEntry_->getPort());
    return client.execute(cmd, args, result);
}

bool
RunnerItem::getParameters(int streamIndex, int controllerIndex, XmlRpc::XmlRpcValue& definition) const
{
    XmlRpc::XmlRpcValue args;
    args.setSize(2);
    args[0] = streamIndex;
    args[1] = controllerIndex;
    return executeRequest("getParameters", args, definition);
}

bool
RunnerItem::setParameters(int streamIndex, int controllerIndex, const XmlRpc::XmlRpcValue& updates) const
{
    XmlRpc::XmlRpcValue args;
    args.setSize(3);
    args[0] = streamIndex;
    args[1] = controllerIndex;
    args[2] = updates;
    XmlRpc::XmlRpcValue result;
    return executeRequest("setParameters", args, result);
}

bool
RunnerItem::getChangedParameters(QStringList& changes) const
{
    Logger::ProcLog log("getChangedParameters", Log());
    LOGINFO << std::endl;

    XmlRpc::XmlRpcValue args;
    args.setSize(0);
    XmlRpc::XmlRpcValue result;

    bool rc = executeRequest("getChangedParameters", args, result);
    if (!rc) {
        LOGERROR << "failed XML-RPC for changed parameters" << std::endl;
        changes.append("*** failed XML-RPC for changed parameters ***\n");
        return false;
    }

    LOGDEBUG << "result: " << result.toXml() << std::endl;
    for (int index = 0; index < getNumChildren(); ++index) {
        int row = streamIndexMapping_[index];
        const XmlRpc::XmlRpcValue& streamChanges(result[index]);
        LOGDEBUG << index << " streamChanges: " << streamChanges.toXml() << std::endl;
        getChild(row)->formatChangedParameters(streamChanges, changes);
    }

    return true;
}

StreamItem*
RunnerItem::getChild(int index) const
{
    return static_cast<StreamItem*>(Super::getChild(index));
}

void
RunnerItem::setServiceEntry(ServiceEntry* serviceEntry)
{
    static Logger::ProcLog log("setServiceEntry", Log());
    LOGINFO << getServiceName() << ' ' << serviceEntry_ << ' ' << serviceEntry << std::endl;
    serviceEntry_ = serviceEntry;
    if (serviceEntry_) {
        log_ = RunnerLog::Find(serviceEntry->getName());
    } else {
        log_ = 0;
    }
}

void
RunnerItem::afterUpdate()
{
    static Logger::ProcLog log("afterUpdate", Log());

    QString name(QString::fromStdString(getStatus().getName()));
    LOGDEBUG << "our name: " << getName() << " status name: " << name << std::endl;
    if (getName() != name) {
        setName(name);
        LOGWARNING << "name changed: " << name << std::endl;
    }

    Super::afterUpdate();

    if (!log_) log_ = RunnerLog::Find(getServiceName());
    if (log_) log_->appendLogMessages(getStatus().getLogMessages());
}
