#ifndef SIDECAR_GUI_TASKITEM_H // -*- C++ -*-
#define SIDECAR_GUI_TASKITEM_H

#include "IO/TaskStatus.h"

#include "TreeViewItem.h"

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {
namespace Master {

class StreamItem;

/** TreeViewItem that represents an IO::Task object of an IO::Stream.
 */
class TaskItem : public TreeViewItem {
    Q_OBJECT
    using Super = TreeViewItem;

public:
    static Logger::Log& Log();

    /** Constructor.

        \param status initial status value of the item

        \param parent parent node for this item
    */
    TaskItem(const IO::TaskStatus& status, StreamItem* parent);

    void updateCollectionStats(CollectionStats& stats) const;

    QVariant getNameDataValue(int role) const;

    QVariant getStateDataValue(int role) const;

    QVariant getPendingCountValue(int role) const;

    /** Obtain the data value for the fourth column, which displays the message processing rate of the TaskItem.
        Shows the current message count, message rate, and byte rate.

        \param role the display role that determines the type of data to return

        \return returns count + rate information
    */
    QVariant getRateDataValue(int role) const;

    QVariant getErrorDataValue(int role) const;

    QVariant getInfoDataValue(int role) const;

    IO::ProcessingState::Value getProcessingState() const { return getStatus().getProcessingState(); }

    const QString& getError() const { return error_; }

    const QString& getConnectionInfo() const { return connectionInfo_; }

    /** Obtain the message count from the last status entry.

        \return message count
    */
    int getMessageCount() const { return getStatus().getMessageCount(); }

    /** Obtain the byte rate from the last status container.

        \return byte rate
    */
    int getByteRate() const { return getStatus().getByteRate(); }

    /** Obtain the message rate from the last status container.

        \return message rate
    */
    int getMessageRate() const { return getStatus().getMessageRate(); }

    virtual int getDropCount() const { return getStatus().getDropCount(); }

    virtual int getDupeCount() const { return getStatus().getDupeCount(); }

    int getPendingQueueCount() const { return getStatus().getPendingQueueCount(); }

    bool isUsingData() const { return getStatus().isUsingData(); }

    StreamItem* getParent() const;

    void formatChangedParameters(const XmlRpc::XmlRpcValue& definitions, QStringList& changes) const;

    QString getParameterChangedHeading() const;

protected:
    void beforeUpdate();

    void afterUpdate();

    virtual void fillCollectionStats(CollectionStats& stats) const;

private:
    /** Obtain the type-cast status container sent by a IO::Task object.

        \return read-only IO::TaskStatus reference
    */
    const IO::TaskStatus& getStatus() const { return getStatusT<IO::TaskStatus>(); }

    int lastMessageCount_; ///< Message count from last status
    QString error_;
    QString connectionInfo_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
