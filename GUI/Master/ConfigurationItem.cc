#include "GUI/LogUtils.h"
#include "GUI/ServiceEntry.h"
#include "XMLRPC/XmlRpcClient.h"
#include "XMLRPC/XmlRpcValue.h"

#include "ConfigurationItem.h"
#include "RootItem.h"
#include "RunnerItem.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;
using namespace SideCar::Runner;

Logger::Log&
ConfigurationItem::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.ConfigurationItem");
    return log_;
}

ConfigurationItem::ConfigurationItem(const RunnerStatus& status, RootItem* parent) :
    Super(status, parent), streamCount_(0)
{
    setName(QString::fromStdString(status.getConfigName()));
}

void
ConfigurationItem::childAdded(TreeViewItem* child)
{
    streamCount_ += child->getNumChildren();
}

void
ConfigurationItem::childRemoved(TreeViewItem* child)
{
    streamCount_ -= child->getNumChildren();
}

RunnerItem*
ConfigurationItem::findService(const QString& serviceName) const
{
    // Visit each RunnerItem, returning the first (and only) one that has the given service name. Remember that
    // Zeroconf disambiguates duplicate publisher names, so this uniqueness constraint shall always remain
    // valid.
    //
    for (int index = 0; index < getNumChildren(); ++index) {
        RunnerItem* item = getChild(index);
        if (item->getServiceName() == serviceName) return item;
    }
    return 0;
}

RunnerItem*
ConfigurationItem::findService(const ServiceEntry* serviceEntry) const
{
    return findService(serviceEntry->getName());
}

RunnerItem*
ConfigurationItem::getChild(int index) const
{
    return static_cast<RunnerItem*>(Super::getChild(index));
}

bool
ConfigurationItem::executeRequest(const char* cmd, const XmlRpc::XmlRpcValue& args) const
{
    static Logger::ProcLog log("executeRequest", Log());

    bool ok = true;
    for (int index = getNumChildren() - 1; index >= 0; --index) {
        ServiceEntry* serviceEntry = getChild(index)->getServiceEntry();
        if (!serviceEntry) continue;

        LOGDEBUG << "cmd: " << cmd << ' ' << serviceEntry->getHost() << '/' << serviceEntry->getPort() << std::endl;

        XmlRpc::XmlRpcClient client(serviceEntry->getHost().toStdString(), serviceEntry->getPort());
        XmlRpc::XmlRpcValue result;
        if (!client.execute(cmd, args, result)) ok = false;
    }

    return ok;
}

bool
ConfigurationItem::update(const Runner::RunnerStatus& status, RunnerItem* runnerItem)
{
    beforeUpdate();
    bool changed = runnerItem->update(status);
    afterUpdate();
    return changed;
}

bool
ConfigurationItem::getChangedParameters(QStringList& changes) const
{
    bool ok = true;
    for (int index = 0; index < getNumChildren(); ++index) {
        if (!getChild(index)->getChangedParameters(changes)) ok = false;
    }
    return ok;
}
