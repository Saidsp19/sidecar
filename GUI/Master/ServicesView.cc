#include "QtCore/QSettings"
#include "QtCore/QSignalMapper"
#include "QtGui/QAction"
#include "QtGui/QContextMenuEvent"
#include "QtGui/QHBoxLayout"
#include "QtGui/QFrame"
#include "QtGui/QHeaderView"
#include "QtGui/QItemDelegate"
#include "QtGui/QMenu"
#include "QtGui/QMessageBox"
#include "QtGui/QToolButton"

#include "GUI/LogUtils.h"

#include "ConfigurationItem.h"
#include "ControllerItem.h"
#include "MainWindow.h"
#include "ParamEditor.h"
#include "RunnerItem.h"
#include "RunnerLog.h"
#include "ServicesModel.h"
#include "ServicesView.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

Logger::Log&
ServicesView::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.ServicesView");
    return log_;
}

/** Item delegate for the "Name" column. Takes into account installed indexWidget() widgets when calculating
 * sizeHint() values.
 */
struct NameItemDelegate : public QItemDelegate
{
    using Super = QItemDelegate;
    
    NameItemDelegate(ServicesView* view) : Super(view), view_(view) {}

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;

    ServicesView* view_;
};

QSize
NameItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                           const QModelIndex& index) const
{
    QWidget* w = view_->indexWidget(index);
    if (w)
	return w->sizeHint();
    else
	return Super::sizeHint(option, index);
}

ServicesView::ServicesView(QWidget* parent)
    : Super(parent), columnVisibility_(ServicesModel::kNumColumns, true),
      contextMenu_(new QMenu(this)),
      contextMenuMapper_(new QSignalMapper(this)), adjustmentTimer_(),
      known_(), visible_(), filter_(""), dirtyColumns_()
{
    Logger::ProcLog log("ServicesView", Log());

    adjustmentTimer_.setInterval(0);
    adjustmentTimer_.setSingleShot(true);

    dirtyColumns_.resize(ServicesModel::kNumColumns);

    connect(&adjustmentTimer_, SIGNAL(timeout()),
            SLOT(doAdjustColumnSizes()));

    connect(this, SIGNAL(expanded(const QModelIndex&)),
            SLOT(itemExpanded(const QModelIndex&)));

    connect(this, SIGNAL(collapsed(const QModelIndex&)),
            SLOT(itemCollapsed(const QModelIndex&)));

    // Install delegate for the "Name" column that will take into account any indexWidget() widgets when
    // calculating item sizeHint() values.
    //
    setItemDelegateForColumn(ServicesModel::kName,
                             new NameItemDelegate(this));

#if 0
    header()->setStyleSheet(
	"QHeaderView::section {"
	"background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop: 0 "
	"#616161, stop: 0.5 #505050, stop: 0.6 #434343, stop: 1 #656565);"
	"color: #CCFFCC;"
	"border: 1px solid #6c6c6c;"
	"}");
#endif

    // header()->setResizeMode(QHeaderView::ResizeToContents);

    QSettings settings;
    columnVisibility_ =
	settings.value("ColumnVisibility", columnVisibility_).toBitArray();

    connect(contextMenuMapper_, SIGNAL(mapped(int)),
            SLOT(toggleColumnVisibility(int)));
}

void
ServicesView::setModel(QAbstractItemModel* model)
{
    Logger::ProcLog log("setModel", Log()); 

    Super::setModel(model);
    for (int index = 0; index < ServicesModel::kNumColumns; ++index) {
	bool isVisible = columnVisibility_.testBit(index);
	setColumnHidden(index, ! isVisible);
    }

    QAction* action;

    action = new QAction("Show Runner Log", this);
    connect(action, SIGNAL(triggered()), SLOT(showRunnerLog()));
    contextMenu_->addAction(action);

    action = new QAction("Shutdown Runner", this);
    connect(action, SIGNAL(triggered()), SLOT(shutdownRunner()));
    contextMenu_->addAction(action);

    contextMenu_->addSeparator();

    action = new QAction("Expand Hierarchy", this);
    connect(action, SIGNAL(triggered()), SLOT(expandHierarchy()));
    contextMenu_->addAction(action);

    action = new QAction("Collapse Hierarchy", this);
    connect(action, SIGNAL(triggered()), SLOT(collapseHierarchy()));
    contextMenu_->addAction(action);

    contextMenu_->addSeparator();

    action = new QAction("Expand Configurations", this);
    connect(action, SIGNAL(triggered()), SLOT(expandConfigurations()));
    contextMenu_->addAction(action);

    action = new QAction("Collapse Configurations", this);
    connect(action, SIGNAL(triggered()), SLOT(collapseConfigurations()));
    contextMenu_->addAction(action);

    contextMenu_->addSeparator();

    action = new QAction("Expand Runners", this);
    connect(action, SIGNAL(triggered()), SLOT(expandRunners()));
    contextMenu_->addAction(action);

    action = new QAction("Collapse Runners", this);
    connect(action, SIGNAL(triggered()), SLOT(collapseRunners()));
    contextMenu_->addAction(action);

    contextMenu_->addSeparator();

    action = new QAction("Expand Streams", this);
    connect(action, SIGNAL(triggered()), SLOT(expandStreams()));
    contextMenu_->addAction(action);

    action = new QAction("Collapse Streams", this);
    connect(action, SIGNAL(triggered()), SLOT(collapseStreams()));
    contextMenu_->addAction(action);

    contextMenu_->addSeparator();

    QMenu* menu = new QMenu("Column Visibility", contextMenu_);
    for (int index = 0; index < ServicesModel::kNumColumns; ++index) {
	bool isVisible = columnVisibility_.testBit(index);
	LOGDEBUG << index << ' ' << isVisible << std::endl;
	action = new QAction(ServicesModel::GetColumnName(index), this);
	action->setCheckable(true);
	action->setChecked(isVisible);
	contextMenuMapper_->setMapping(action, index);
	connect(action, SIGNAL(toggled(bool)), contextMenuMapper_,
                SLOT(map()));
	menu->addAction(action);
    }

    contextMenu_->addMenu(menu);
}

void
ServicesView::traverseAndSet(const QModelIndex& index, bool expand)
{
    static Logger::ProcLog log("traverseAndSet", Log());
    TreeViewItem* item = ServicesModel::GetModelData(index);
    if (! item) return;

    LOGINFO << "item: " << item->getFullName() << ' ' << item->canExpand()
	    << std::endl;
    if (item->canExpand()) {
	setExpanded(index, expand);
	QModelIndex child = index.child(0, 0);
	while (child.isValid()) {
	    traverseAndSet(child, expand);
	    child = child.sibling(child.row() + 1, 0);
	}
    }
}

void
ServicesView::initializeRow(const QModelIndex& index)
{
    static Logger::ProcLog log("traverseAndExpand", Log());
    TreeViewItem* item = ServicesModel::GetModelData(index);
    if (! item) return;

    LOGINFO << "item: " << item->getFullName() << ' ' << item->canExpand()
	    << std::endl;

    if (item->canExpand()) {
	setAnimated(false);
	if (getWasExpanded(index))
	    setExpanded(index, true);
	else
	    setExpanded(index, false);
	setAnimated(true);
	QModelIndex child = index.child(0, 0);
	while (child.isValid()) {
	    initializeRow(child);
	    child = child.sibling(child.row() + 1, 0);
	}
    }

    if (item->canEdit() && ! indexWidget(index)) {

	// Create a new push button for the algorithm that when clicked will show a dialog box with the current
	// runtime parammeter values.
	//
	QToolButton* button = new QToolButton(this);
	button->setObjectName("paramEditor");
	button->setText(item->getName());
	setIndexWidget(index, button);

	// Connect the button to a new ParamEditor object.
	//
	ParamEditor* editor = new ParamEditor(
	    qobject_cast<MainWindow*>(window()),
	    qobject_cast<ControllerItem*>(item));
	connect(button, SIGNAL(clicked()), editor, SLOT(beginEdit()));
    }
}

void
ServicesView::rowsInserted(const QModelIndex& parent, int start, int end)
{
    static Logger::ProcLog log("rowsInserted", Log());
    LOGINFO << "parent.isValid: " << parent.isValid()
	    << " start: " << start << " end: " << end << std::endl;

    // Let the super class handle view adjustments before we start mucking with the expansion state of the new
    // entries.
    //
    Super::rowsInserted(parent, start, end);

    // Iterate over the new rows, and expand the new row and its children (streams).
    //
    for (; start <= end; ++start) {
	QModelIndex index(model()->index(start, 0, parent));
	LOGDEBUG << "index.isValid: " << index.isValid() << std::endl;
	TreeViewItem* item = ServicesModel::GetModelData(index);
	Q_ASSERT(item);
	LOGDEBUG << "item: " << item->getFullName() << std::endl;
	ConfigurationItem* cfg = qobject_cast<ConfigurationItem*>(item);
	initializeRow(index);
	if (cfg)
	    setRowHidden(start, parent,
                         getConfigurationIsHidden(index, cfg));
    }
}

void
ServicesView::dataChanged(const QModelIndex& topLeft,
                          const QModelIndex& bottomRight)
{
    Super::dataChanged(topLeft, bottomRight);
    for (int index = topLeft.column(); index < bottomRight.column(); ++index)
	dirtyColumns_[index] = true;
    adjustColumnSizes(false);
}

void
ServicesView::toggleColumnVisibility(int index)
{
    Logger::ProcLog log("toggleColumnVisibility", Log());
    columnVisibility_.toggleBit(index);
    bool isVisible = columnVisibility_.testBit(index);
    LOGINFO << index << ' ' << isVisible << std::endl;
    setColumnHidden(index, ! isVisible);
    QSettings settings;
    settings.setValue("ColumnVisibility", columnVisibility_);
}

void
ServicesView::contextMenuEvent(QContextMenuEvent* event)
{
    Logger::ProcLog log("contextMenuEvent", Log());
    LOGINFO << std::endl;
    contextMenuItem_ = indexAt(event->pos());
    const TreeViewItem* obj = contextMenuItem_.isValid() ?
	ServicesModel::GetModelData(contextMenuItem_) : 0;

    // The first two entries in the context menu are only valid for RunnerItem objects. A QModelIndex for a
    // RunnerItem does not have a valid grandparent index.
    //
    QList<QAction*> actions = contextMenu_->actions();

    bool enabled;
    if (! contextMenuItem_.parent().isValid())
	enabled = false;
    else if (! contextMenuItem_.parent().parent().isValid())
	enabled = true;
    else
	enabled = false;
    
    actions[0]->setEnabled(obj && enabled);
    actions[1]->setEnabled(obj && enabled);

    // The expand/collapse hierarchy entries are only valid for entries with children.
    //
    enabled = obj && obj->getNumChildren() > 0;
    actions[3]->setEnabled(enabled);
    actions[4]->setEnabled(enabled);

    contextMenu_->exec(event->globalPos());
}

void
ServicesView::showRunnerLog()
{
    RunnerItem* runnerItem = qobject_cast<RunnerItem*>(
	ServicesModel::GetModelData(contextMenuItem_));
    if (runnerItem) {
	RunnerLog* log = RunnerLog::Find(runnerItem->getServiceName());
	if (log) log->showWindow();
    }
}

void
ServicesView::shutdownRunner()
{
    RunnerItem* runnerItem = qobject_cast<RunnerItem*>(
	ServicesModel::GetModelData(contextMenuItem_));
    if (! runnerItem) return;

    if (QMessageBox::question(
            qApp->activeWindow(), "Runner Shutdown",
	    QString("<p>Are you sure you want to shut down the runner "
                    "<b>%1</b>? Doing so will leave the configuration "
                    "'%2' in an partial state.</p>")
            .arg(runnerItem->getName())
            .arg(runnerItem->getConfigName()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==
        QMessageBox::Yes) {

	XmlRpc::XmlRpcValue args;
	XmlRpc::XmlRpcValue result;
	runnerItem->executeRequest("shutdown", args, result);
    }
}

void
ServicesView::expandHierarchy()
{
    traverseAndSet(contextMenuItem_, true);
}

void
ServicesView::collapseHierarchy()
{
    traverseAndSet(contextMenuItem_, false);
}

void
ServicesView::expandConfigurations()
{
    QModelIndex index(model()->index(0, 0));
    while (index.isValid()) {
	if (! isExpanded(index))
	    setExpanded(index, true);
	index = index.sibling(index.row() + 1, 0);
    }
}

void
ServicesView::collapseConfigurations()
{
    QModelIndex index(model()->index(0, 0));
    while (index.isValid()) {
	if (isExpanded(index))
	    setExpanded(index, false);
	index = index.sibling(index.row() + 1, 0);
    }
}

void
ServicesView::expandRunners()
{
    QModelIndex index(model()->index(0, 0));
    while (index.isValid()) {
	QModelIndex child(index.child(0, 0));
	while (child.isValid()) {
	    if (! isExpanded(child))
		setExpanded(child, true);
	    child = child.sibling(child.row() + 1, 0);
	}
	index = index.sibling(index.row() + 1, 0);
    }
}

void
ServicesView::collapseRunners()
{
    QModelIndex index(model()->index(0, 0));
    while (index.isValid()) {
	QModelIndex child(index.child(0, 0));
	while (child.isValid()) {
	    if (isExpanded(child))
		setExpanded(child, false);
	    child = child.sibling(child.row() + 1, 0);
	}
	index = index.sibling(index.row() + 1, 0);
    }
}

void
ServicesView::expandStreams()
{
    QModelIndex index(model()->index(0, 0));
    while (index.isValid()) {
	QModelIndex child(index.child(0, 0));
	while (child.isValid()) {
	    QModelIndex grandChild(child.child(0, 0));
	    while (grandChild.isValid()) {
		if (! isExpanded(grandChild))
		    setExpanded(grandChild, true);
		grandChild = grandChild.sibling(grandChild.row() + 1, 0);
	    }
	    child = child.sibling(child.row() + 1, 0);
	}
	index = index.sibling(index.row() + 1, 0);
    }
}

void
ServicesView::collapseStreams()
{
    QModelIndex index(model()->index(0, 0));
    while (index.isValid()) {
	QModelIndex child(index.child(0, 0));
	while (child.isValid()) {
	    QModelIndex grandChild(child.child(0, 0));
	    while (grandChild.isValid()) {
		if (isExpanded(grandChild))
		    setExpanded(grandChild, false);
		grandChild = grandChild.sibling(grandChild.row() + 1, 0);
	    }
	    child = child.sibling(child.row() + 1, 0);
	}
	index = index.sibling(index.row() + 1, 0);
    }
}

void
ServicesView::adjustColumnSizes(bool all)
{
    static Logger::ProcLog log("adjustColumnSizes", Log());
    LOGINFO << std::endl;

    if (all) {
	dirtyColumns_.clear();
	dirtyColumns_.resize(ServicesModel::kNumColumns, true);
    }

    if (! adjustmentTimer_.isActive())
	adjustmentTimer_.start();
}

void
ServicesView::doAdjustColumnSizes()
{
    static Logger::ProcLog log("doAdjustColumnSizes", Log());
    LOGINFO << std::endl;
    for (int index = 0; index < ServicesModel::kNumColumns; ++index) {
	if (dirtyColumns_[index]) {
	    resizeColumnToContents(index);
	    dirtyColumns_[index] = false;
	    if (index == ServicesModel::kName)
		setColumnWidth(index, columnWidth(index) + 20);
	}
    }
}

void
ServicesView::itemExpanded(const QModelIndex& index)
{
    static Logger::ProcLog log("itemExpanded", Log());
    TreeViewItem* item = ServicesModel::GetModelData(index);
    if (! item) return;
    LOGINFO << "row: " << index.row() << " item: " << item
	    << " name: " << item->getFullName() << std::endl;
    QSettings settings;
    settings.setValue(item->getFullName(), true);
    item->setExpanded(true);
    adjustColumnSizes(true);
}

void
ServicesView::itemCollapsed(const QModelIndex& index)
{
    static Logger::ProcLog log("itemCollapsed", Log());
    TreeViewItem* item = ServicesModel::GetModelData(index);
    if (! item) return;
    LOGINFO << "row: " << index.row() << " item: " << item
	    << " name: " << item->getFullName() << std::endl;
    QSettings settings;
    settings.setValue(item->getFullName(), false);
    item->setExpanded(false);
    adjustColumnSizes(true);
}

bool
ServicesView::getWasExpanded(const QModelIndex& index) const
{
    static Logger::ProcLog log("getWasExpanded", Log());
    TreeViewItem* item = ServicesModel::GetModelData(index);
    if (! item) return false;
    LOGINFO << "row: " << index.row() << " item: " << item
            << " name: " << item->getFullName() << std::endl;
    QSettings settings;
    bool value = settings.value(item->getFullName(), true).toBool();
    LOGDEBUG << "name: " << item->getFullName() << ' ' << value << std::endl;
    return value;
}

void
ServicesView::setConfigurationVisibleFilter(const QStringList& known,
                                            const QStringList& visible,
                                            const QString& filter)
{
    static Logger::ProcLog log("setConfigurationVisibleFilter", Log());
    LOGINFO << "filter: " << filter << std::endl;

    known_ = known;
    visible_ = visible;
    filter_ = filter;

    // Iterate over the top-level items, which are ConfigurationItems objects.
    //
    QModelIndex index(model()->index(0, 0));
    while (index.isValid()) {
	TreeViewItem* item = ServicesModel::GetModelData(index);
	ConfigurationItem* cfg = qobject_cast<ConfigurationItem*>(item);
	Q_ASSERT(cfg);

	bool isHidden = getConfigurationIsHidden(index, cfg);
	setRowHidden(index.row(), index.parent(), isHidden);

	index = index.sibling(index.row() + 1, 0);
    }

    adjustColumnSizes(true);
}

void
ServicesView::showTreeViewItem(const QModelIndex& index)
{
    setRowHidden(index.row(), index.parent(), false);
    QModelIndex child(index.child(0, 0));
    while (child.isValid()) {
	showTreeViewItem(child);
	child = child.sibling(child.row() + 1, 0);
    }
}

bool
ServicesView::getTreeViewItemIsHidden(const QModelIndex& index)
{
    static Logger::ProcLog log("getTreeViewItemIsHidden", Log());
    TreeViewItem* item = ServicesModel::GetModelData(index);
    LOGINFO << "filter: " << filter_ << " index: " << index.row() << ','
	    << index.column() << " name: " << item->getName() << std::endl;

    bool isHidden = true;
    if (item->isFiltered(filter_)) {
	LOGDEBUG << "matched" << std::endl;
	showTreeViewItem(index);
	isHidden = false;
    }
    else {
	QModelIndex child(index.child(0, 0));
	while (child.isValid()) {
	    bool hide = getTreeViewItemIsHidden(child);
	    if (! hide) {
		LOGDEBUG << "child " << child.row() << " matched" << std::endl;
		isHidden = false;
	    }
	    setRowHidden(child.row(), index, hide);
	    child = child.sibling(child.row() + 1, 0);
	}
    }

    LOGDEBUG << "returning " << isHidden << std::endl;
    return isHidden;
}

bool
ServicesView::getConfigurationIsHidden(const QModelIndex& index,
                                       const ConfigurationItem* item)
{
    QString name(item->getName());

    // If there is a configuration file, but the configuration does not have its visible bit set, then hide it.
    //
    if (filter_.isEmpty() && known_.contains(name) &&
        ! visible_.contains(name))
	return true;
    return getTreeViewItemIsHidden(index);
}

void
ServicesView::setExpanded(const QModelIndex& index, bool expanded)
{
    TreeViewItem* item = ServicesModel::GetModelData(index);
    if (item) item->setExpanded(expanded);
    Super::setExpanded(index, expanded);
}
