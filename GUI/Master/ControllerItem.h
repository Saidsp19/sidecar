#ifndef SIDECAR_GUI_CONTROLLERITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONTROLLERITEM_H

#include "boost/shared_ptr.hpp"

#include "Algorithms/ControllerStatus.h"

#include "TaskItem.h"

namespace SideCar {
namespace GUI {
namespace Master {

class InfoFormatter;

/** Derivation of TaskItem that represents an Algorithms::Controller object of an IO::Stream. Redefines the
    display in the recording status and info data columns. The status object from a Algorithms::Controller
    object contains the name of an InfoFormatter object to use to format info data from the Algorithm object
    managed by the controller.

    Also supports runtime parameter queries and updates.
*/
class ControllerItem : public TaskItem {
    Q_OBJECT
    using Super = TaskItem;

public:
    /** Constructor.

        \param status initial status value of the item

        \param parent parent node for this item
    */
    ControllerItem(const Algorithms::ControllerStatus& status, StreamItem* parent);

    ~ControllerItem();

    QVariant getNameDataValue(int role) const;

    /** Override of TreeViewItem method. Obtain a representation of the recording status of the controller.

        \param role the display role that determines the type of data to return

        \return returns a character icon depicting the recording status.
    */
    QVariant getRecordingDataValue(int role) const;

    QVariant getPendingCountValue(int role) const;

    QVariant getRateDataValue(int role) const;

    /** Override of TreeViewItem method. Obtain the data value for the last column, which displays
        Algorithm-specific information.

        \param role the display role that determines the type of data to return

        \return formatted Algorithm data if role is Qt::DisplayRole
    */
    QVariant getInfoDataValue(int role) const;

    bool canRecord() const { return true; }

    bool willRecord() const { return getStatus().isRecordingEnabled(); }

    bool isRecording() const { return getStatus().isRecordingOn(); }

    /** Override of TreeViewItem method. Determine if the object supports editing of runtime parameter values.

        \return true if so
    */
    bool canEdit() const { return getStatus().hasParameters(); }

    int getRecordingQueueCount() const { return getStatus().getRecordingQueueCount(); }

    /** Obtain the current runtime parameter values from the XML-RPC server of the runner process hosting the
        Controller.

        \param definition container to hold the returned value

        \return true if successful
    */
    bool getParameters(XmlRpc::XmlRpcValue& definition) const;

    /** Submit new runtime parameter values to the XML-RPC server of the runner process hosting the Controller.

        \param updates container holding the new settings

        \return true if successful
    */
    bool setParameters(const XmlRpc::XmlRpcValue& updates) const;

    QString getFormattedProcessingTime(double time) const;

    int getDropCount() const { return 0; }

    int getDupeCount() const { return 0; }

protected:
    void fillCollectionStats(CollectionStats& stats) const;

    /** Override of TreeViewItem method. Verifies that the held InfoFormatter object is valid for the new
        status.
    */
    void afterUpdate();

    /** Obtain the type-cast status container sent by a Algorithms::Controller object.

        \return read-only Algorithms::ControllerStatus reference
    */
    const Algorithms::ControllerStatus& getStatus() const { return getStatusT<Algorithms::ControllerStatus>(); }

private:
    InfoFormatter* infoFormatter_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
