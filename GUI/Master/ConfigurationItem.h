#ifndef SIDECAR_GUI_CONFIGURATIONITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGURATIONITEM_H

#include <vector>

#include "Runner/RunnerStatus.h"

#include "CollectionItem.h"

namespace Logger {
class Log;
}
namespace XmlRpc {
class XmlRpcValue;
}

namespace SideCar {
namespace GUI {

class ServiceEntry;

namespace Master {

class RootItem;
class RunnerItem;

/** TreeViewItem that represents a configuration file, which contains a collection of runner processes.
 */
class ConfigurationItem : public CollectionItem {
    Q_OBJECT
    using Super = CollectionItem;

public:
    static Logger::Log& Log();

    /** Constructor for a new configuration collection

        \param status initial status value of the item

        \param serviceEntry connection information for the XML-RPC server of
        the runner process. May be NULL if the ServiceEntry is not yet
        available.

        \param parent parent node for this item
    */
    ConfigurationItem(const Runner::RunnerStatus& status, RootItem* parent);

    RunnerItem* findService(const QString& serviceName) const;

    RunnerItem* findService(const ServiceEntry* serviceEntry) const;

    /** Shadow of TreeViewItem method that returns a type-casted value

        \param index chld to obtain

        \return RunnerItem object
    */
    RunnerItem* getChild(int index) const;

    bool update(const Runner::RunnerStatus& status, RunnerItem* runnerItem);

    /** Issue an XML-RPC request to the runner application.

        \param cmd remote command to execute

        \param args command arguments

        \param result command result

        \return true if successfully executed
    */
    bool executeRequest(const char* cmd, const XmlRpc::XmlRpcValue& args) const;

    int getStreamCount() const { return streamCount_; }

    bool getChangedParameters(QStringList& changes) const;

private:
    void childAdded(TreeViewItem* child);

    void childRemoved(TreeViewItem* child);

    void updateChildren() {}

    int streamCount_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
