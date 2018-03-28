#ifndef SIDECAR_GUI_TREEVIEWITEM_H // -*- C++ -*-
#define SIDECAR_GUI_TREEVIEWITEM_H

#include "QtCore/QList"
#include "QtCore/QVariant"
#include "QtGui/QColor"
#include "QtGui/QFont"

#include "IO/StatusBase.h"

namespace SideCar {
namespace GUI {
namespace Master {

class CollectionStats;

/** Base class for a row in a QTreeView display. All TreeViewItem objects have a parent (though the parent may
    be NULL), and a TreeViewItem may have children. Supports adding and removing of children and fetching of a
    particular child by index. Defines the display interface used by ServicesModel to obtain the data to display
    in a column.

    The TreeViewItem::getData() method is the main entry point for data
    retrieval from an item. It is called by the ServicesModel::data() method.
    Internally, getData() switches on the column index, calling the appropriate
    virtual method for the column. There are five columns
    (ServicesModel::kNumColumns) of data, and thus five methods:

    - getNameDataValue
    - getStateDataValue
    - getRecordingDataValue
    - getRateDataValue
    - getInfoDataValue

    Each method takes a display role as its sole argument, which specify the
    type of data to return. If a method has no value to return, it must return
    a null QVariant instance, obtained by QVariant(). If the method returns
    a null QVariant, getData() will, for certain display roles, attempt to get
    values from the following virtual methods:

    - getAlignment
    - getFont
    - getForegroundColor

    Each method takes a column index as its sole argument.
*/
class TreeViewItem : public QObject {
    Q_OBJECT
public:
    using Children = QList<TreeViewItem*>;
    using ChildrenIterator = QListIterator<TreeViewItem*>;

    enum ActiveState {
        kIdle,        ///< Item has not processed data recently
        kPartial,     ///< Some but not all chilren are kIdle
        kActive,      ///< Item or all children are processing data
        kNotUsingData ///< Item is not pulling data
    };

    /** Utility class method that returns an appropriate value for the recording state depicted by the two
        arguments

        \param canRecord true if the item is enabled for recording

        \param role the display role of the data to return

        \return display data
    */
    static QVariant GetRecordingDataValue(bool canRecord, int role);

    /** Class method that returns the default (black) text color for all Qt::Foreground roles.

        \return black color
    */
    static QColor GetTextColor();

    /** Class method that returns the OK text color for those Qt::Foreground roles that distinguish among an
        'OK', 'warning', or 'failure' state.

        \return dark green color
    */
    static QColor GetOKColor();

    /** Class method that returns the 'warning text color for those Qt::Foreground roles that distinguish among
        an 'OK', 'warning', or 'failure' state.

        \return dark yellow color
    */
    static QColor GetWarningColor();

    /** Class method that returns the 'failure' text color for those Qt::Foreground roles that distinguish among
        an 'OK', 'warning', or 'failure' state.

        \return dark red color
    */
    static QColor GetFailureColor();

    /** Class method that returns the text color to use to show when a task is currently recording.

        \return dark red color
    */
    static QColor GetRecordingColor();

    /** Obtain the normal font to use for displayed text.

        \return QFont value
    */
    static QFont GetTextFont();

    /** Destructor. Deletes all held children.
     */
    ~TreeViewItem();

    /** Initialize this object's internal state. Derived classes may override. This implementation simply
        invokes beforeUpdate() followed by afterUpdate().
    */
    virtual void initialize()
    {
        beforeUpdate();
        afterUpdate();
    }

    /** Obtain the latest IO::StatusBase object received for this object.

        \return IO::StatusBase reference.
    */
    const IO::StatusBase& getStatus() const { return status_; }

    /** Update the TreeViewItem held status. If the new status is different than the held status value, calls
        performUpdate().

        \param status new status value to install

        \return true if new status was changed
    */
    bool update(const IO::StatusBase& status);

    bool isOK() const { return ok_; }

    /** Determine if the object is in a 'processing' state: auto-diagnostic, calibration, or run

        \return true if so, false otherwise
    */
    bool isProcessing() const { return processing_; }

    /** Obtain the current ActiveState value for this object.

        \return ActiveState value
    */
    ActiveState getActiveState() const { return activeState_; }

    /** Obtain the data point to use for a specific role in a given column.

        \param column the column to fetch

        \param role the display role that determines the type of data to return

        \return found value or QVariant() if nothing special to display
    */
    QVariant getData(int column, int role) const;

    /** Obtain the data value for the ServicesModel::kName column, which displays the name of the TreeViewItem.

        \param role the display role that determines the type of data to return

        \return object name if role is Qt::DisplayRole
    */
    virtual QVariant getNameDataValue(int role) const;

    /** Obtain the data value for the ServicesModel::kHost column, which displays the host name of the machine
        where the object is running.

        \param role the display role that determines the type of data to return

        \return null QVariant
    */
    virtual QVariant getHostDataValue(int role) const { return QVariant(); }

    /** Obtain the data value for the ServicesModel::kState column, which displays the processing state of the
        TreeViewItem.

        \param role the display role that determines the type of data to return

        \return processing state name if role is Qt::DisplayRole
    */
    virtual QVariant getStateDataValue(int role) const;

    /** Obtain the data value for the ServicesModel::kRecording column, which displays the recording state of
        the TreeViewItem.

        \param role the display role that determines the type of data to return

        \return null QVariant
    */
    virtual QVariant getRecordingDataValue(int role) const;

    /** Obtain the data value for the ServicesModel::kPending column, which displays the pending queue size of
        this item.

        \param role the display role that determines the type of data to return

        \return null QVariant

        \return null QVariant
    */
    virtual QVariant getPendingCountValue(int role) const;

    /** Obtain the data value for the ServicesModel::kRate column, which displays the message processing rate of
        the TreeViewItem.

        \param role the display role that determines the type of data to return

        \return returns "Active" if the message counts of the last two status
        updates are different.
    */
    virtual QVariant getRateDataValue(int role) const;

    /** Obtain the data value for the ServicesModel::kError column, which displays any error text from the
        TreeViewItem.

        \param role the display role that determines the type of data to return

        \return null QVariant
    */
    virtual QVariant getErrorDataValue(int role) const;

    /** Obtain the data value for the ServicesModel::kInfo column, which displays miscellaneous informational
        data of the TreeViewItem.

        \param role the display role that determines the type of data to return

        \return null QVariant
    */
    virtual QVariant getInfoDataValue(int role) const;

    /** Obtain the foreground color to use for the contents of a given column.

        \param column the column to work on

        \return QColor value
    */
    virtual QColor getForegroundColor(int column) const;

    /** Obtain the font to use for the contents of a given column.

        \param column the column to work on

        \return QFont value
    */
    virtual QFont getFont(int column) const;

    /** Obtain the alignment (horizontal and vertical) to use for the contents of a given column.

        \param column the column to work on

        \return alignment flags
    */
    virtual int getAlignment(int columnt) const;

    /** Determine if the TreeViewItem supports editing of runtime parameter values. Derived classes must
        redefine if they do.

        \return always returns false
    */
    virtual bool canEdit() const { return false; }

    /** Determine if the TreeViewItem is expandable.

        \return true if so.
    */
    virtual bool canExpand() const { return false; }

    /** Obtain the object's name.

        \return name value
    */
    const QString& getName() const { return name_; }

    /** Get the full name of the item. This is a colon-separated list of names that depict the inheritance
        relationship for this item. The first name in the list is the name of a RunnerItem. The last name is
        this items's name.

        \return full name
    */
    const QString& getFullName() const { return fullName_; }

    /** Insert a child, renumbering the indices of all children after it

        \param index where to insert

        \param child the TreeViewItem to remember
    */
    void insertChild(int row, TreeViewItem* child);

    /** Add a child.

        \param child the TreeViewItem to remember
    */
    void appendChild(TreeViewItem* child);

    /** Remove the child at a given index.

        \param index which child to remove
    */
    void removeChild(int index);

    /** Obtain the child associated with a given index value.

        \param index which child to return

        \return TreeViewItem pointer
    */
    TreeViewItem* getChild(int index) const { return children_[index]; }

    /** Obtain the number of children.

        \return child count
    */
    int getNumChildren() const { return children_.size(); }

    /** Obtain the index of this object in the context of our parent's children array.

        \return index of self
    */
    int getIndex() const { return index_; }

    /** Obtain the parent of this TreeViewItem object.

        \return TreeViewItem object (may be NULL)
    */
    TreeViewItem* getParent() const { return parent_; }

    /** Obtain the index value of the parent object (equivalent to getParent()->getIndex().

        \return index of parent
    */
    int getParentIndex() const { return parent_->index_; }

    /** Obtain a Java-style iterator for the children array.

        \return iterator
    */
    ChildrenIterator getChildrenIterator() const { return children_; }

    /** Determine if the object is expanded

        \return true if so
    */
    bool isExpanded() const { return expanded_; }

    /** Change the expansion attribute of the object

        \param expanded new state
    */
    void setExpanded(bool expanded) { expanded_ = expanded; }

    /** Prototype for method to update a CollectionStats object with statistics provided by a TreeViewItem
        derivative. Derived classes must define.

        \param stats object to update
    */
    virtual void updateCollectionStats(CollectionStats& stats) const = 0;

    /** Obtain the row where the given name could be inserted without disturbing the ordering of the existing
        rows. Uses the name_ attribute of its children to determine ordering.

        \param name the value to check

        \return location to insert, 0 for beginning of children_ and
        children_.size() for end.
    */
    int getInsertionPoint(const QString& name) const;

    /** Determine if this item contains the given filter string. This implementation looks in the item's name_
        attribute. Derivations are free to choose alternative filtering means, though they should always call
        this implementation for uniform behavior.

        \param filter the text to look for

        \return true if found
    */
    virtual bool isFiltered(const QString& filter) const { return name_.contains(filter, Qt::CaseInsensitive); }

protected:
    /** Constructor for the RootItem object (which has no parent).
     */
    TreeViewItem();

    /** Constructor for all derived classes except RootItem.

        \param status the initial status value for the object

        \param parent the parent of the item
    */
    TreeViewItem(const IO::StatusBase& status, TreeViewItem* parent);

    void setName(const QString& name);

    void setOK(bool ok) { ok_ = ok; }

    void setProcessing(bool processing) { processing_ = processing; }

    void setProcessingState(IO::ProcessingState::Value state);

    void setActiveState(ActiveState activeState) { activeState_ = activeState; }

    /** Hook invoked before a status update. This implementation does nothing.
     */
    virtual void beforeUpdate() {}

    /** Hook invoked after a status update. This implementation does nothing.
     */
    virtual void afterUpdate() {}

    template <typename T>
    const T& getStatusT() const
    {
        return static_cast<const T&>(getStatus());
    }

    virtual void childAdded(TreeViewItem* child) {}

    virtual void childRemoved(TreeViewItem* child) {}

private:
    IO::StatusBase status_; ///< Last received status container
    TreeViewItem* parent_;  ///< Parent object for this one
    int index_;             ///< Index of this object in the parent
    Children children_;     ///< Collection of children
    QString name_;          ///< Name of this object
    QString fullName_;      ///< Name of this object
    bool ok_;
    bool processing_;
    ActiveState activeState_;
    bool expanded_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
