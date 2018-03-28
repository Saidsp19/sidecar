#ifndef SIDECAR_GUI_RUNNERITEM_H // -*- C++ -*-
#define SIDECAR_GUI_RUNNERITEM_H

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

class ConfigurationItem;
class RunnerLog;
class StreamItem;

/** TreeViewItem that represents a runner process, which contains a collection of processing streams. When a
    ServicesModel receives a status container, it creates new RunnerItem objects when it fails to find the
    status container name. The RunnerItem holds a pointer to a GUI::ServiceEntry, which contains information
    about a Zeroconf-discovered service. When a RunnerItem has a valid ServiceEntry, the ServicesModel can talk
    to it via XML-RPC requests.
*/
class RunnerItem : public CollectionItem {
    Q_OBJECT
    using Super = CollectionItem;

public:
    static Logger::Log& Log();

    /** Constructor for a new runner process

        \param status initial status value of the item

        \param serviceEntry connection information for the XML-RPC server of
        the runner process. May be NULL if the ServiceEntry is not yet
        available.

        \param parent parent node for this item
    */
    RunnerItem(const Runner::RunnerStatus& status, ConfigurationItem* parent);

    /** Obtain the data value for the Host column, which displays host names where runner programs are
        executing.

        \param role the display role that determines the type of data to return

        \return display value
    */
    QVariant getHostDataValue(int role) const;

    /** Obtain the data value for the Info column, which displays task-specific information.

        \param role the display role that determines the type of data to return

        \return display value
    */
    QVariant getInfoDataValue(int role) const;

    void setServiceEntry(ServiceEntry* serviceEntry);

    /** Obtain the service entry found for this runner. NOTE: this value may be NULL if we have not yet received
        a resolved ServiceEntry object.

        \return ServiceEntry object or NULL if none assigned
    */
    ServiceEntry* getServiceEntry() const { return serviceEntry_; }

    /** Obtain the unique name for the runner. This is a composite of the configuration name, host name, and
        runner name, separated by ':' charadcters.

        \return unique service name
    */
    QString getServiceName() const { return QString::fromStdString(getStatus().getServiceName()); }

    /** Obtain the name of the host where the runner is executing.

        \return host name
    */
    QString getHostName() const { return QString::fromStdString(getStatus().getHostName()); }

    /** Obtain the name of the configuration in use by the remote runner process.

        \return configuration name
    */
    QString getConfigName() const { return QString::fromStdString(getStatus().getConfigName()); }

    /** Obtain the name of the configuration in use by the remote runner process.

        \return configuration name
    */
    QString getLogPath() const { return QString::fromStdString(getStatus().getLogPath()); }

    /** Shadow of TreeViewItem method that returns a type-casted value

        \param index chld to obtain

        \return StreamItem object
    */
    StreamItem* getChild(int index) const;

    /** Obtain the current runtime parameter values from the XML-RPC server of the runner process hosting the
        Controller.

        \param definition container to hold the returned value

        \return true if successful
    */
    bool getParameters(int streamIndex, int taskIndex, XmlRpc::XmlRpcValue& definition) const;

    /** Submit new runtime parameter values to the XML-RPC server of the runner process hosting the Controller.

        \param changes container holding the new settings

        \return true if successful
    */
    bool setParameters(int streamIndex, int taskIndex, const XmlRpc::XmlRpcValue& changes) const;

    /** Obtain a list of parameter settings that differ from their original settings as defined in the XML
        configuration file used to start the Runner.

        \param definition XML container that will contain the changes

        \return true if successful
    */
    bool getChangedParameters(QStringList& changes) const;

    /** Issue an XML-RPC request to the runner application.

        \param cmd remote command to execute

        \param args command arguments

        \param result command result

        \return true if successfully executed
    */
    bool executeRequest(const char* cmd, const XmlRpc::XmlRpcValue& args, XmlRpc::XmlRpcValue& result) const;

    /** Override of TreeViewItem method. Allow filter matching on the host name where the Runner is executing.

        \param filter the text used as a filter

        \return true if this item matches the filter
    */
    bool isFiltered(const QString& filter) const
    {
        return Super::isFiltered(filter) || getHostName().contains(filter, Qt::CaseInsensitive);
    }

protected:
    /** Override of CollectionItem::afterUpdate(). Assigns RunnerLog objects if necessary, and then posts log
        messages from the last status update to the assigned RunnerLog object.
    */
    void afterUpdate();

private:
    /** Obtain the type-cast status container sent by a Algorithms::Runner object.

        \return read-only RunnerStatus reference
    */
    const Runner::RunnerStatus& getStatus() const { return getStatusT<Runner::RunnerStatus>(); }

    /** Update the status for the streams managed by the runner process.
     */
    void updateChildren();

    ServiceEntry* serviceEntry_; ///< XML-RPC server connection info
    QString displayName_;
    RunnerLog* log_;
    std::vector<int> streamIndexMapping_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
