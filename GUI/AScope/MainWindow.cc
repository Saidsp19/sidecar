#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtGui/QCloseEvent"
#include "QtGui/QStatusBar"

#include "GUI/LogUtils.h"
#include "GUI/Splittable.h"
#include "GUI/ToolBar.h"
#include "GUI/Utils.h"
#include "GUI/WindowManager.h"
#include "Messages/Video.h"

#include "AzimuthLatch.h"
#include "CursorWidget.h"
#include "DisplayView.h"
#include "History.h"
#include "HistoryControlWidget.h"
#include "HistoryPosition.h"
#include "HistorySettings.h"
#include "RadarInfoWidget.h"
#include "MainWindow.h"
#include "PeakBarSettings.h"
#include "ViewChooser.h"
#include "ViewSettings.h"
#include "Visualizer.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::AScope;
using namespace SideCar::Messages;

Logger::Log&
MainWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.MainWindow");
    return log_;
}

MainWindow::MainWindow(History& history, int windowIndex, DisplayView* basis)
    : MainWindowBase(), Ui::MainWindow(), activeDisplayView_(0),
      displayViews_()
{
    static Logger::ProcLog log("MainWindow", Log());
    LOGINFO << basis << std::endl;

    // Build the widgets. Delete everything when the window closes.
    //
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

#ifdef __DEBUG__
    setWindowTitle(QString("AScope (DEBUG) - %1").arg(windowIndex));
#else
    setWindowTitle(QString("AScope - %1").arg(windowIndex));
#endif

    App* app = getApp();

    // Associate icons with QAction objects that toggle data display
    //
    {
	QIcon icon;
	icon.addFile(":/showGates.png", QSize(16, 16), QIcon::Normal,
                     QIcon::Off);
	icon.addFile(":/showRanges.png", QSize(16, 16), QIcon::Normal,
                     QIcon::On);
	actionToggleHorizontalScale_->setIcon(icon);
    }

    {
	QIcon icon;
	icon.addFile(":/showSamples.png", QSize(16, 16), QIcon::Normal,
                     QIcon::Off);
	icon.addFile(":/showVoltages.png", QSize(16, 16), QIcon::Normal,
                     QIcon::On);
	actionToggleVerticalScale_->setIcon(icon);
    }

    actionViewToggleGrid_->setCheckable(true);
    actionViewToggleGrid_->setIcon(QIcon(":/gridOn.png"));
    actionViewToggleGrid_->setChecked(true);
    UpdateToggleAction(actionViewToggleGrid_, true);

    actionViewTogglePeaks_->setCheckable(true);
    actionViewTogglePeaks_->setIcon(QIcon(":/peaksOn.png"));

    actionViewPause_->setCheckable(true);
    actionViewPause_->setData(QStringList() << "Freeze" << "Unfreeze");
    actionViewPause_->setIcon(QIcon(":/unfreeze.png"));
    actionViewFull_->setIcon(QIcon(":/home.png"));

    ToolBar* toolBar;

    actionViewTogglePeaks_->setEnabled(app->getPeakBarSettings().isEnabled());
    connect(&app->getPeakBarSettings(), SIGNAL(enabledChanged(bool)),
            SLOT(peakBarsEnabledChanged(bool)));

    // Cursor info widget
    //
    cursorWidget_ = new CursorWidget(statusBar());
    statusBar()->addPermanentWidget(cursorWidget_);

    // Radar info widget
    //
    radarInfoWidget_ = new RadarInfoWidget(statusBar());
    statusBar()->addPermanentWidget(radarInfoWidget_);

    // View Editor/Chooser toolbar
    //
    toolBar = makeToolBar("View Chooser", Qt::TopToolBarArea);
    toolBar->toggleVisibility();
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    QAction* action = app->getToolsMenuAction(App::kShowViewEditor);
    action->setIcon(QIcon(":/viewEditor.svg"));
    toolBar->addAction(action);
    ViewChooser* chooser = new ViewChooser(app->getViewEditor(), toolBar);
    toolBar->addWidget(chooser);

    // Azimuth Trigger config toolbar
    //
    toolBar = makeToolBar("Azimuth Latch", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    azimuthLatch_ = new AzimuthLatch(toolBar);
    toolBar->addWidget(azimuthLatch_);

    // History control toolbar
    //
    toolBar = makeToolBar("History Control", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    historyControlWidget_ = new HistoryControlWidget(toolBar);
    toolBar->addWidget(historyControlWidget_);

    // Tool windows toolbar
    //
    toolBar = makeToolBar("Tool Windows", Qt::LeftToolBarArea);
    action = app->getToolsMenuAction(App::kShowChannelConnectionWindow);
    action->setIcon(QIcon(":/channelsWindow.png"));
    toolBar->addAction(action);

    action = app->getToolsMenuAction(App::kShowConfigurationWindow);
    action->setIcon(QIcon(":/configurationWindow.png"));
    toolBar->addAction(action);
    
    // View actions toolbar
    //
    toolBar = makeToolBar("View", Qt::LeftToolBarArea);
    toolBar->addAction(actionToggleHorizontalScale_);
    toolBar->addAction(actionToggleVerticalScale_);
    toolBar->addAction(actionViewToggleGrid_);
    toolBar->addAction(actionViewTogglePeaks_);
    toolBar->addAction(actionViewPause_);
    toolBar->addAction(actionViewFull_);
    actionViewPrevious_->setIcon(QIcon(":/popViewStack.png"));
    toolBar->addAction(actionViewPrevious_);
    actionViewSwap_->setIcon(QIcon(":/refresh.png"));
    toolBar->addAction(actionViewSwap_);
    app->addWindowMenuActionTo(toolBar, WindowManager::kFullScreen);

    actionViewSplitHorizontally_->setIcon(
	QIcon(":/horizontalSplitView.png"));
    toolBar->addAction(actionViewSplitHorizontally_);
    actionViewSplitVertically_->setIcon(
	QIcon(":/verticalSplitView.png"));
    toolBar->addAction(actionViewSplitVertically_);
    actionViewUnsplit_->setIcon(
	QIcon(":/unSplitView.png"));
    toolBar->addAction(actionViewUnsplit_);

    // Create content. We use a Splittable widget so that the user can split the view it contains into two, the
    // new views becoming Splittable as well.
    //
    Splittable* splittable = new Splittable(this);
    splittable->setContents(makeDisplayView(splittable, basis));
    setCentralWidget(splittable);
    actionViewPrevious_->setEnabled(false);
    actionViewSwap_->setEnabled(false);
}

void
MainWindow::saveSplittable(QSettings& settings, Splittable* splittable)
{
    static Logger::ProcLog log("saveSplittable", Log());
    LOGINFO << splittable << std::endl;

    // Write out the current settings for the given Splittable object so that they may be used in the future to
    // restore the application to the same state it was in prior to quitting.
    //
    QSplitter* splitter = splittable->getSplitter();
    bool isSplit = splitter;
    settings.setValue("IsSplit", isSplit);
    if (isSplit) {

	// Save the sizes of the two views. The settings for the DisplayView widgets will be saved in the
	// recursive invocations of saveSplittable() below.
	//
	QList<int> sizes = splitter->sizes();
	int orientation = splitter->orientation();
	settings.setValue("Orientation", orientation);
	settings.setValue("FirstSize", sizes[0]);
	settings.setValue("SecondSize", sizes[1]);
	settings.beginGroup("Left");
	saveSplittable(settings, splittable->topLeft()); // !!! recursion
	settings.endGroup();
	settings.beginGroup("Right");
	saveSplittable(settings, splittable->bottomRight()); // !!! recursion
	settings.endGroup();
    }
    else {

	// Just save the DisplayView widget settings.
	//
	settings.beginGroup("DisplayView");
	DisplayView* displayView = dynamic_cast<DisplayView*>(
	    splittable->currentWidget());
	Q_ASSERT(displayView);
	displayView->saveToSettings(settings);
	settings.endGroup();
    }
}

void
MainWindow::restoreSplittable(QSettings& settings, Splittable* splittable)
{
    // Restore split views per a settings file.
    //
    bool isSplit = settings.value("IsSplit").toBool();
    if (isSplit) {
	Qt::Orientation orientation = Qt::Orientation(
	    settings.value("Orientation").toInt());

	// Get the size of the two views.
	//
	int firstSize = settings.value("FirstSize").toInt();
	int secondSize = settings.value("SecondSize").toInt();
	splittable->split(orientation,
                          makeDisplayView(0, activeDisplayView_), firstSize,
                          secondSize);
	settings.beginGroup("Left");
	restoreSplittable(settings, splittable->topLeft()); // !!! recursion
	settings.endGroup();
	settings.beginGroup("Right");
	restoreSplittable(settings, splittable->bottomRight()); // !!!
	settings.endGroup();
    }
    else {

	// Restore the DisplayvView widget settings held by the Splittable widget.
	//
	settings.beginGroup("DisplayView");
	DisplayView* displayView = dynamic_cast<DisplayView*>(
	    splittable->currentWidget());
	Q_ASSERT(displayView);
	displayView->restoreFromSettings(settings);
	settings.endGroup();
    }
}

void
MainWindow::saveToSettings(QSettings& settings)
{
    static Logger::ProcLog log("saveToSettings", Log());
    LOGINFO << settings.group() << std::endl;

    // Save the Splittable widget and its children.
    //
    Splittable* splittable = static_cast<Splittable*>(centralWidget());
    saveSplittable(settings, splittable);

    // Save the window location and size.
    //
    Super::saveToSettings(settings);
    LOGINFO << "done" << std::endl;
}

void
MainWindow::restoreFromSettings(QSettings& settings)
{
    static Logger::ProcLog log("restoreFromSettings", Log());
    LOGINFO << settings.group() << std::endl;

    // Restore the Splittable widget settings and recreate any child split views.
    //
    Splittable* splittable = static_cast<Splittable*>(centralWidget());
    restoreSplittable(settings, splittable);
    if (! displayViews_.empty())
	displayViews_.back()->getVisualizer()->setFocus();
    actionViewUnsplit_->setEnabled(isDisplayViewSplit());

    // Restore the window location and size.
    //
    Super::restoreFromSettings(settings);
    LOGINFO << "done" << std::endl;
}

QWidget*
MainWindow::makeDisplayView(QWidget* parent, DisplayView* basis)
{
    static Logger::ProcLog log("makeDisplayView", Log());
    LOGDEBUG << parent << ' ' << basis << std::endl;

    // Create a new DisplayView, possibly inheriting some view settings from another view as is the case when
    // splitting a view.
    //
    DisplayView* displayView = new DisplayView(parent, azimuthLatch_);
    if (basis) displayView->duplicate(basis);

    // Receive a notification when a DisplayView goes away or becomes the active view.
    //
    connect(displayView, SIGNAL(destroyed(QObject*)),
            SLOT(displayViewDeleted(QObject*)));
    connect(displayView, SIGNAL(activeDisplayViewChanged(DisplayView*)),
            SLOT(activeDisplayViewChanged(DisplayView*)));

    // Connect the drawing view to our cursor display to show the current cursor position at the bottom of the
    // window.
    //
    Visualizer* visualizer = displayView->getVisualizer();
    connect(visualizer,
            SIGNAL(pointerMoved(const QPoint&, const QPointF&)),
            cursorWidget_,
            SLOT(showCursorPosition(const QPoint&, const QPointF&)));

    // Update buttons and menu settings when the geometric transform of drawing view changes.
    //
    connect(visualizer, SIGNAL(transformChanged()),
            SLOT(updateViewActions()), Qt::QueuedConnection);

    // Connect the HistoryPosition widget and the drawing view.
    //
    HistoryPosition* historyPosition = visualizer->getHistoryPosition();
    historyControlWidget_->manage(historyPosition);
    connect(displayView, SIGNAL(activeDisplayViewChanged(DisplayView*)),
            historyControlWidget_,
            SLOT(activeDisplayViewChanged(DisplayView*)));
    connect(historyPosition, SIGNAL(viewingPastChanged(bool)),
            actionViewPause_, SLOT(setChecked(bool)));
    connect(historyPosition,
            SIGNAL(infoUpdate(const Messages::PRIMessage::Ref&)),
            radarInfoWidget_,
            SLOT(showMessageInfo(const Messages::PRIMessage::Ref&)));

    LOGDEBUG << "new DisplayView: " << displayView << std::endl;

    displayViews_.append(displayView);
    getApp()->addDisplayView(displayView);

    visualizer->setFocus();

    // Make sure we always have an activeDisplayView_ value
    //
    if (! activeDisplayView_)
	displayView->setActiveDisplayView();

    return displayView;
}

void
MainWindow::displayViewDeleted(QObject* obj)
{
    static Logger::ProcLog log("displayViewDeleted", Log());
    LOGINFO << obj << std::endl;
    int index = displayViews_.indexOf(static_cast<DisplayView*>(obj));
    Q_ASSERT(index != -1);
    displayViews_.erase(displayViews_.begin() + index);
    if (! displayViews_.empty())
	displayViews_.back()->getVisualizer()->setFocus();
}

void
MainWindow::splitDisplayViewHorizontally()
{
    splitDisplayView(Qt::Horizontal);
}

void
MainWindow::splitDisplayViewVertically()
{
    splitDisplayView(Qt::Vertical);
}

Splittable*
MainWindow::splitDisplayView(Qt::Orientation how)
{
    if (! activeDisplayView_) return 0;
    Splittable* splittable =
	dynamic_cast<Splittable*>(activeDisplayView_->parentWidget());
    if (! splittable) return 0;
    splittable->split(how, makeDisplayView(0, activeDisplayView_));
    return splittable;
}

void
MainWindow::on_actionViewToggleGrid__triggered()
{
    bool newState = ! activeDisplayView_->isShowingGrid();
    activeDisplayView_->setShowGrid(newState);
    UpdateToggleAction(actionViewToggleGrid_, newState);
}

void
MainWindow::on_actionViewTogglePeaks__triggered()
{
    bool newState = ! activeDisplayView_->isShowingPeakBars();
    activeDisplayView_->setShowPeakBars(newState);
    UpdateToggleAction(actionViewTogglePeaks_, newState);
}

void
MainWindow::on_actionViewPause__triggered()
{
    bool newState = ! activeDisplayView_->isFrozen();
    activeDisplayView_->setFrozen(newState);
    UpdateToggleAction(actionViewPause_, newState);
}

void
MainWindow::on_actionViewFull__triggered()
{
    Visualizer* visualizer = activeDisplayView_->getVisualizer();
    visualizer->popAllViews();
    actionViewPrevious_->setEnabled(false);
    actionViewSwap_->setEnabled(false);
    actionViewFull_->setEnabled(false);
}

void
MainWindow::on_actionViewPrevious__triggered()
{
    Visualizer* visualizer = activeDisplayView_->getVisualizer();
    visualizer->popView();
    actionViewPrevious_->setEnabled(visualizer->canPopView());
    actionViewSwap_->setEnabled(visualizer->canPopView());
    actionViewFull_->setEnabled(visualizer->canPopView());
}

void
MainWindow::on_actionViewSwap__triggered()
{
    Visualizer* visualizer = activeDisplayView_->getVisualizer();
    visualizer->swapViews();
}

void
MainWindow::on_actionViewSplitHorizontally__triggered()
{
    splitDisplayViewHorizontally();
    actionViewUnsplit_->setEnabled(isDisplayViewSplit());
}

void
MainWindow::on_actionViewSplitVertically__triggered()
{
    splitDisplayViewVertically();
    actionViewUnsplit_->setEnabled(isDisplayViewSplit());
}

void
MainWindow::on_actionViewUnsplit__triggered()
{
    unsplitDisplayView();
    actionViewUnsplit_->setEnabled(isDisplayViewSplit());
}

bool
MainWindow::isDisplayViewSplit() const
{
    Splittable* splittable = getActiveSplittable();
    if (! splittable) return false;
    QObject* parent = splittable->parentWidget();
    if (! parent) return false;
    return dynamic_cast<QSplitter*>(parent);
}

void
MainWindow::unsplitDisplayView()
{
    Splittable* splittable = getActiveSplittable();
    if (! splittable) return;
    splittable->closeOtherView();
}

Splittable*
MainWindow::getActiveSplittable() const
{
    if (! activeDisplayView_) return 0;
    return dynamic_cast<Splittable*>(activeDisplayView_->parentWidget());
}

void
MainWindow::activeDisplayViewChanged(DisplayView* displayView)
{
    static Logger::ProcLog log("activeDisplayViewChanged", Log());
    LOGINFO << "new display view: " << displayView << std::endl;

    if (activeDisplayView_) {

	// Disconnect the old display view
	//
	Visualizer* visualizer = activeDisplayView_->getVisualizer();
	HistoryPosition* historyPosition = visualizer->getHistoryPosition();
	historyPosition->setInfoReporter(false);
    }

    // Connect the new display view.
    //
    activeDisplayView_ = displayView;
    if (displayView) {
	Visualizer* visualizer = activeDisplayView_->getVisualizer();
	visualizer->setFocus();
	HistoryPosition* historyPosition = visualizer->getHistoryPosition();
	historyPosition->setInfoReporter(true);

	actionToggleHorizontalScale_->setEnabled(true);
	actionToggleVerticalScale_->setEnabled(true);

	actionToggleHorizontalScaleUpdate(
	    visualizer->getCurrentView().isShowingRanges());
	actionToggleVerticalScaleUpdate(
	    visualizer->getCurrentView().isShowingVoltages());

	actionViewUnsplit_->setEnabled(isDisplayViewSplit());
	actionViewSplitVertically_->setEnabled(true);
	actionViewSplitHorizontally_->setEnabled(true);

	actionViewToggleGrid_->setEnabled(true);
	UpdateToggleAction(actionViewToggleGrid_,
                           activeDisplayView_->isShowingGrid());

	actionViewTogglePeaks_->setEnabled(true);
	UpdateToggleAction(actionViewTogglePeaks_,
                           activeDisplayView_->isShowingPeakBars());

	actionViewPause_->setEnabled(true);
	UpdateToggleAction(actionViewPause_,
                           activeDisplayView_->isFrozen());
    }
    else {
	actionToggleHorizontalScale_->setEnabled(false);
	actionToggleVerticalScale_->setEnabled(false);
	actionViewToggleGrid_->setEnabled(false);
	actionViewTogglePeaks_->setEnabled(false);
	actionViewPause_->setEnabled(false);
    }

    updateViewActions();
}

void
MainWindow::updateViewActions()
{
    if (activeDisplayView_) {
	Visualizer* visualizer = activeDisplayView_->getVisualizer();
	actionViewPrevious_->setEnabled(visualizer->canPopView());
	actionViewSwap_->setEnabled(visualizer->canPopView());
	actionViewFull_->setEnabled(visualizer->canPopView());
	const ViewSettings& viewSettings(visualizer->getCurrentView());
	actionToggleHorizontalScaleUpdate(viewSettings.isShowingRanges());
	actionToggleVerticalScaleUpdate(viewSettings.isShowingVoltages());
    }
    else {
	actionViewPrevious_->setEnabled(false);
	actionViewSwap_->setEnabled(false);
	actionViewFull_->setEnabled(false);
    }	
}

void
MainWindow::peakBarsEnabledChanged(bool state)
{
    actionViewTogglePeaks_->setEnabled(state);
}

void
MainWindow::on_actionToggleHorizontalScale__triggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    bool state = action->isChecked();
    actionToggleHorizontalScaleUpdate(state);
    activeDisplayView_->getVisualizer()->setShowingRanges(state);
}

void
MainWindow::actionToggleHorizontalScaleUpdate(bool state)
{
    if (state != actionToggleHorizontalScale_->isChecked())
	actionToggleHorizontalScale_->setChecked(state);
    if (state) {
	actionToggleHorizontalScale_->setToolTip(
	    "Horizontal scale: range distances\n"
	    "Click to show range indices");
    }
    else {
	actionToggleHorizontalScale_->setToolTip(
	    "Horizontal scale: range indices\n"
	    "Click to show range distances");
    }
}

void
MainWindow::on_actionToggleVerticalScale__triggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    bool state = action->isChecked();
    actionToggleVerticalScaleUpdate(state);
    activeDisplayView_->getVisualizer()->setShowingVoltages(state);
}

void
MainWindow::actionToggleVerticalScaleUpdate(bool state)
{
    if (state != actionToggleVerticalScale_->isChecked())
	actionToggleVerticalScale_->setChecked(state);
    if (state) {
	actionToggleVerticalScale_->setToolTip(
	    "Vertical scale: voltage values\n"
	    "Click to show sample counts");
    }
    else {
	actionToggleVerticalScale_->setToolTip(
	    "Vertical scale: sample counts\n"
	    "Click to show voltage values");
    }
}

void
MainWindow::windowMenuAboutToShow(QList<QAction*>& actions)
{
    Super::windowMenuAboutToShow(actions);
    actions[WindowManager::kNew]->setEnabled(true);
}
