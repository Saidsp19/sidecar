#ifndef SIDECAR_GUI_STREAMITEM_H // -*- C++ -*-
#define SIDECAR_GUI_STREAMITEM_H

#include "IO/StreamStatus.h"

#include "CollectionItem.h"

namespace Logger {
class Log;
}
namespace XmlRpc {
class XmlRpcValue;
}

namespace SideCar {
namespace GUI {
namespace Master {

class RunnerItem;
class TaskItem;

/** TreeViewItem that represents an IO::Stream object of a runner process. An IO::Stream contains IO::Modules,
    which is a simple container for an IO::Task object.
*/
class StreamItem : public CollectionItem {
    Q_OBJECT
    using Super = CollectionItem;

public:
    static Logger::Log& Log();

    /** Constructor.

        \param status initial status value of the item

        \param parent parent node for this item
    */
    StreamItem(const IO::StreamStatus& status, RunnerItem* parent);

    /** Obtain the item's RunnerItem parent.

        \return RunnerItem object
    */
    RunnerItem* getParent() const;

    /** Obtain a TaskItem child.

        \param index which TaskItem to fetch

        \return TaskItem object
    */
    TaskItem* getChild(int index) const;

    /** Obtain an XML description of the runtime parameter values for a particular child TaskItem.

        \param taskIndex which TaskItem to work with

        \param definition XML container update with the XML description

        \return true if successful
    */
    bool getParameters(int taskIndex, XmlRpc::XmlRpcValue& definition) const;

    /** Change one or more runtime parameter values of a particular child TaskItem.

        \param taskIndex which TaskItem to work with

        \param definition XML container containing the parameter changes to
        apply

        \return true if successful
    */
    bool setParameters(int taskIndex, const XmlRpc::XmlRpcValue& updates) const;

    void formatChangedParameters(const XmlRpc::XmlRpcValue& definitions, QStringList& changes) const;

private:
    /** Obtain the type-cast status container sent by a IO::Streams object.

        \return read-only IO::StreamStatus reference
    */
    const IO::StreamStatus& getStatus() const { return static_cast<const IO::StreamStatus&>(Super::getStatus()); }

    /** Update the status for the tasks managed by the stream.
     */
    void updateChildren();

    int streamIndex_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
