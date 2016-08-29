#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtGui/QAction"
#include "QtGui/QCloseEvent"
#include "QtGui/QIcon"
#include "QtGui/QStatusBar"

#include "GUI/LogUtils.h"
#include "GUI/modeltest.h"
#include "GUI/PresetManager.h"
#include "GUI/ToolBar.h"
#include "GUI/Utils.h"
#include "GUI/WindowManager.h"

#include "ChannelConnection.h"
#include "ChannelConnectionModel.h"
#include "ChannelPlotSettings.h"
#include "ChannelPlotWidget.h"
#include "ConfigurationWindow.h"
#include "MainWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::HealthAndStatus;

Logger::Log&
MainWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("hands.MainWindow");
    return log_;
}

MainWindow::MainWindow()
    : MainWindowBase(), Ui::MainWindow()
{
    static Logger::ProcLog log("MainWindow", Log());
    LOGINFO << std::endl;
    setupUi(this);
    setObjectName("MainWindow");
    setWindowTitle("Health and Status");
    model_ = new ChannelConnectionModel(this);
#ifdef __DEBUG__
    new ModelTest(model_, this);
#endif
    toolBar_->addWidget(new QLabel("Unconnected", this));
    unconnected_ = new QComboBox(this);
    unconnected_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    toolBar_->addWidget(unconnected_);

    QAction* action = getApp()->getConfigurationWindow()->getShowHideAction();
    action->setIcon(QIcon(":/configurationWindow.png"));
    toolBar_->insertAction(actionClear_, action);

    plots_->setModel(model_);

    connect(plots_->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,
                                    const QItemSelection&)),
            SLOT(currentPlotWidgetChanged(const QItemSelection&,
                                          const QItemSelection&)));

    connect(model_, SIGNAL(availableServicesChanged(const QStringList&)),
            SLOT(setAvailableNames(const QStringList&)));

    updateButtons();
}

void
MainWindow::addPlot(const QString& name)
{
    Logger::ProcLog log("addPlot", Log());
    LOGINFO << "name: " << name << std::endl;

    QModelIndexList selection = getSelectedPlotWidgets();
    int row = model_->rowCount();
    ChannelPlotSettings* basis =
	getApp()->getConfigurationWindow()->getDefaultSettings();
    if (! selection.empty()) {
	row = selection.back().row();
	basis = model_->getConnection(row)->getSettings();
    }

    LOGDEBUG << "row: " << row << std::endl;

    ChannelConnection* obj = new ChannelConnection(name, this);
    connect(getApp(), SIGNAL(shutdown()), obj, SLOT(shutdown()));
    obj->getSettings()->copyFrom(basis);

    ChannelPlotWidget* widget = new ChannelPlotWidget(obj, plots_);

    obj->setMinimumSizeHint(widget->minimumSizeHint());

    QModelIndex index = model_->add(obj, row);
    plots_->setIndexWidget(index, widget);
    if (row < model_->rowCount() - 1)
	setCurrentPlotWidget(row);
}

void
MainWindow::on_actionAdd__triggered()
{
    int index = unconnected_->currentIndex();
    if (index == -1) return;
    QString name = unconnected_->currentText();
    addPlot(name);
    unconnected_->removeItem(index);
    updateButtons();
}

void
MainWindow::on_actionRemove__triggered()
{
    QStringList names;

    int row = -1;
    QModelIndexList indices = getSelectedPlotWidgets();
    while (! indices.empty()) {
	row = indices.back().row();
	indices.pop_back();
	names.append(model_->getConnection(row)->getName());
	model_->removeRows(row, 1);
    }

    if (row >= model_->rowCount())
	row = model_->rowCount() - 1;

    if (row != -1)
	setCurrentPlotWidget(row);

    int unconnectedIndex = unconnected_->currentIndex();
    for (int index = 0; index < unconnected_->count(); ++index)
	names.append(unconnected_->itemText(index));

    qSort(names);
    setUnconnected(names);
    if (unconnectedIndex == -1)
	unconnectedIndex = 0;
    unconnected_->setCurrentIndex(unconnectedIndex);

    updateButtons();
}

void
MainWindow::on_actionClear__triggered()
{
    on_plots__clearAll();
}

void
MainWindow::on_actionMoveUp__triggered()
{
    Logger::ProcLog log("on_actionMoveUp__triggered", Log());

    foreach (QModelIndex index, getSelectedPlotWidgets()) {
	int row = index.row();
	if (row > 0)
	    model_->moveUp(row);
    }

    updateButtons();
}

void
MainWindow::on_actionMoveDown__triggered()
{
    Logger::ProcLog log("on_actionMoveDown__triggered", Log());
    QModelIndexList indices = getSelectedPlotWidgets();
    while (! indices.empty()) {
	int row = indices.back().row();
	indices.pop_back();
	LOGDEBUG << "row: " << row << std::endl;
	if (row >= 0 && row < model_->rowCount() - 1)
	    model_->moveDown(row);
    }

    updateButtons();
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    App* app = getApp();

    // Since we are the only window that should be alive, tell the application to quit. Only do this if the
    // application is not already in the process of shutting down.
    //
    if (! app->isQuitting()) {
	event->ignore();
	QTimer::singleShot(0, app, SLOT(applicationQuit()));
	return;
    }

    Super::closeEvent(event);
}

void
MainWindow::setUnconnected(const QStringList& names)
{
    QString current(unconnected_->currentText());
    unconnected_->clear();
    unconnected_->addItems(names);
    bool enable = ! names.empty();
    unconnected_->setEnabled(enable);
    actionAdd_->setEnabled(enable);
    int index = unconnected_->findText(current);
    if (index != -1)
	unconnected_->setCurrentIndex(index);
    unconnected_->updateGeometry();
    toolBar_->adjustSize();
}

void
MainWindow::setAvailableNames(const QStringList& names)
{
    QString current(unconnected_->currentText());

    unconnected_->clear();
    foreach (QString name, names) {
	if (! model_->hasChannel(name))
	    unconnected_->addItem(name);
    }

    bool enable = ! names.empty();
    unconnected_->setEnabled(enable);
    actionAdd_->setEnabled(enable);
    int index = unconnected_->findText(current);
    if (index != -1)
	unconnected_->setCurrentIndex(index);
    unconnected_->updateGeometry();
    toolBar_->adjustSize();
}

void
MainWindow::currentPlotWidgetChanged(const QItemSelection& selected,
                                     const QItemSelection& deselected)
{
    Logger::ProcLog log("currentPlotWidgetChanged", Log());
    LOGINFO << "selected: " << selected.count() << " deselected: "
            << deselected.count() << std::endl;

    foreach (QModelIndex index, deselected.indexes()) {
	getApp()->getConfigurationWindow()->removeChannelPlotSettings(
	    ChannelConnectionModel::GetObject(index)->getSettings());
    }
    
    foreach (QModelIndex index, selected.indexes()) {
	getApp()->getConfigurationWindow()->addChannelPlotSettings(
	    ChannelConnectionModel::GetObject(index)->getSettings());
    }

    updateButtons();
}

void
MainWindow::updateButtons()
{
    Logger::ProcLog log("updateButtons", Log());
    LOGINFO << "rowCount: " << model_->rowCount() << std::endl;
    plots_->setEnabled(model_->rowCount() > 0);

    actionRemove_->setEnabled(false);
    actionClear_->setEnabled(false);
    actionMoveUp_->setEnabled(false);
    actionMoveDown_->setEnabled(false);

    bool firstSelected = false;
    bool lastSelected = false;

    foreach (QModelIndex index, getSelectedPlotWidgets()) {
	if (! index.isValid())
	    continue;
	if (index.row() == 0)
	    firstSelected = true;
	if (index.row() == model_->rowCount() - 1)
	    lastSelected = true;
	actionClear_->setEnabled(true);
	actionRemove_->setEnabled(true);
	if (index.row() > 0)
	    actionMoveUp_->setEnabled(true);
	if (index.row() < model_->rowCount() - 1)
	    actionMoveDown_->setEnabled(true);
    }

    if (firstSelected)
	actionMoveUp_->setEnabled(false);
    if (lastSelected)
	actionMoveDown_->setEnabled(false);

    unconnected_->setEnabled(unconnected_->count());
    actionAdd_->setEnabled(unconnected_->count() &&
                           unconnected_->currentIndex() >= 0);
}

QModelIndexList
MainWindow::getSelectedPlotWidgets() const
{
    Logger::ProcLog log("getSelectedPlotWidgets", Log());
    QModelIndexList indices = plots_->selectionModel()->selectedIndexes();
    if (! indices.empty())
	qSort(indices.begin(), indices.end());
    return indices;
}

void
MainWindow::setCurrentPlotWidget(int row)
{
    plots_->setCurrentIndex(model_->index(row, 0));
}

void
MainWindow::saveToSettings(QSettings& settings)
{
    Super::saveToSettings(settings);
    int count = model_->rowCount();
    settings.beginWriteArray("Connections", count);
    for (int index = 0; index < count; ++index) {
	settings.setArrayIndex(index);
	settings.setValue("Name", model_->getConnection(index)->getName());
    }
    settings.endArray();
}

void
MainWindow::restoreFromSettings(QSettings& settings)
{
    Super::restoreFromSettings(settings);
    int count = settings.beginReadArray("Connections");
    for (int index = 0; index < count; ++index) {
	settings.setArrayIndex(index);
	QString name = settings.value("Name").toString();

	ChannelConnection* obj = new ChannelConnection(name, this);
	connect(getApp(), SIGNAL(shutdown()), obj, SLOT(shutdown()));

	ChannelPlotWidget* widget = new ChannelPlotWidget(obj, plots_);
	obj->setMinimumSizeHint(widget->minimumSizeHint());

	QModelIndex modelIndex = model_->add(obj, model_->rowCount());
	plots_->setIndexWidget(modelIndex, widget);

	int pos = unconnected_->findText(name);
	if (pos != -1)
	    unconnected_->removeItem(pos);
    }

    settings.endArray();
    updateButtons();

    // Now that we've restored all of the previous ChannelPlotWidget items, we can now restore their last saved
    // configurations.
    //
    getApp()->getPresetManager()->restoreAllPresets();
}

void
MainWindow::on_plots__clearAll()
{
    Logger::ProcLog log("on_plots__clearAll", Log());
    LOGERROR << std::endl;

    QModelIndexList indices = getSelectedPlotWidgets();
    while (! indices.empty()) {
	ChannelPlotWidget* w = qobject_cast<ChannelPlotWidget*>(
	    plots_->indexWidget(indices.back()));
	w->clearAll();
	indices.pop_back();
    }
}


void
MainWindow::on_plots__clearDrops()
{
    Logger::ProcLog log("on_plots__clearDrops", Log());
    LOGERROR << std::endl;

    QModelIndexList indices = getSelectedPlotWidgets();
    while (! indices.empty()) {
	ChannelPlotWidget* w = qobject_cast<ChannelPlotWidget*>(
	    plots_->indexWidget(indices.back()));
	w->clearDrops();
	indices.pop_back();
    }
}

