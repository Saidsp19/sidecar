#ifndef SIDECAR_GUI_SERVICESVIEW_H // -*- C++ -*-
#define SIDECAR_GUI_SERVICESVIEW_H

#include "QtCore/QBitArray"
#include "QtCore/QTimer"
#include "QtGui/QTreeView"

class QSignalMapper;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Master {

class ConfigurationItem;
class MainWindow;
class ParamEditor;

/** Visual representation of the status information received by Runner objects. A Runner object hosts one or
    more processing streams (IO::Stream class) which contain IO::Task-based objects that do the actual data
    processing. Some of the IO::Task objects are Algorithms::Controller objects that manage an
    Algorithms::Algorithm object performs some work on the data. At present, there is at most one runner process
    per host.

    The Runner object contains an XML-RPC server (wrapped in a RemoteController
    object) and a StatusEmitter object that periodically sends out status
    information to active processes running a GUI::StatusCollector. The status
    sent out contains entries for all of the Runner's IO::Stream objects, and
    their IO::Task/Algorithms::Controller objects. This information is held by
    the GUI::ServicesModel object that provides data to this view.

    The tree view shows information in a nested fashion, with contained entries
    indented under their parents. Entries with children may be expanded or
    collapsed by the user -- a collapsed entry hides its children and
    children's children.

    Entries in the view that correspond to Algorithms::Controller objects have
    a QPushButton widget in the name column, which when pushed, allows the user
    to change runtime parameter settings of the algorithm.
*/
class ServicesView : public QTreeView {
    Q_OBJECT
    using Super = QTreeView;

public:
    static Logger::Log& Log();

    /** Constructor. Creates a context menu that allows for expansion / collapsing of all nodes, and the showing
        / hiding of specific columns.

        \param parent owner of this view (for automatic destruction)
    */
    ServicesView(QWidget* parent = 0);

    /** Override of QTreeView method. Install the data model for the view to use. NOTE: most actions on a view
        do nothing or are ignored by the Qt library until a model has been installed. Thus, some view setup
        happens in this method where it will have an effect.

        \param model
    */
    void setModel(QAbstractItemModel* model);

    void setConfigurationVisibleFilter(const QStringList& known, const QStringList& visible, const QString& filter);

    void setExpanded(const QModelIndex& index, bool expanded);

protected slots:

    /** Override of QTreeView method. Notification from the model that one or more rows have been inserted into
        the model. Expands all expandable entries, and adds a QPushButton widget to entries that correspond to
        Algorithms::Controller objects.

        \param parent index of the containing node

        \param start row within the parent of the first inserted value

        \param end row within the parent of the last inserted value
    */
    void rowsInserted(const QModelIndex& parent, int start, int end);

private slots:

    /** Notification from the context menu to change the visibility of a column.

        \param column the column to change
    */
    void toggleColumnVisibility(int column);

    void showRunnerLog();
    void shutdownRunner();

    void expandHierarchy();
    void collapseHierarchy();

    void expandConfigurations();
    void collapseConfigurations();

    void expandRunners();
    void collapseRunners();

    void expandStreams();
    void collapseStreams();

    void adjustColumnSizes(bool all = false);

    void doAdjustColumnSizes();

    void itemExpanded(const QModelIndex& index);

    void itemCollapsed(const QModelIndex& index);

    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

private:
    bool getConfigurationIsHidden(const QModelIndex& index, const ConfigurationItem* item);

    bool getTreeViewItemIsHidden(const QModelIndex& index);

    void showTreeViewItem(const QModelIndex& indx);

    bool getWasExpanded(const QModelIndex& index) const;

    void setHierarchyExpanded(const QModelIndex& index, bool expanded);

    void setStreamsExpanded(const QModelIndex& index, bool expanded);

    /** Override of QTreeView method. Shows the context menu.

        \param event contains the location to place the menu
    */
    void contextMenuEvent(QContextMenuEvent* event);

    void traverseAndSet(const QModelIndex& index, bool expand);

    void initializeRow(const QModelIndex& index);

    QBitArray columnVisibility_;
    QMenu* contextMenu_;
    QSignalMapper* contextMenuMapper_;
    QModelIndex contextMenuItem_;
    QTimer adjustmentTimer_;
    QStringList known_;
    QStringList visible_;
    QString filter_;
    std::vector<bool> dirtyColumns_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
