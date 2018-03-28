#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtGui/QAction"
#include "QtGui/QContextMenuEvent"
#include "QtGui/QDesktopWidget"
#include "QtGui/QLayout"
#include "QtGui/QMenu"
#include "QtGui/QMenuBar"
#include "QtGui/QStatusBar"
#include "QtGui/QWindowStateChangeEvent"

#include "AppBase.h"
#include "LogUtils.h"
#include "MainWindowBase.h"
#include "ToolBar.h"
#include "Utils.h"
#include "WindowManager.h"

using namespace Utils;
using namespace SideCar::GUI;

Logger::Log&
MainWindowBase::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.MainWindowBase");
    return log_;
}

MainWindowBase::MainWindowBase() :
    QMainWindow(), windowMenu_(0), contextMenu_(0), toolBarMenu_(new QMenu("Toolbars")), showAction_(0),
    initialized_(false), restorable_(true), visibleAfterRestore_(true)

{
    Logger::ProcLog log("MainWindowBase", Log());
    LOGINFO << std::endl;
    setIconSize(QSize(16, 16));
}

MainWindowBase::~MainWindowBase()
{
    Logger::ProcLog log("~MainWindowBase", Log());
    LOGINFO << "objectName: " << objectName() << " title: " << windowTitle() << std::endl;
}

AppBase*
MainWindowBase::getApp()
{
    return AppBase::GetApp();
}

void
MainWindowBase::initialize()
{
    Q_ASSERT(!initialized_);

    makeMenuBar();
    if (!contextMenu_) {
        contextMenu_ = new QMenu("Context Menu", this);
        if (contextMenu_) fillContextMenu(contextMenu_);
    }

    initialized_ = true;
}

void
MainWindowBase::makeMenuBar()
{
    QMenu* applicationMenu = getApp()->makeApplicationMenu(this);
    QMenu* toolsMenu = getApp()->makeToolsMenu(this);
    if (!windowMenu_) windowMenu_ = getApp()->makeWindowMenu(this);
    QMenu* loggingMenu = getApp()->makeLoggingMenu(this);
    QMenu* helpMenu = getApp()->makeHelpMenu(this);

    // Always place the application menu before anything else in the menu bar.
    //
    QMenuBar* mb = menuBar();
    QList<QAction*> existing = mb->actions();
    if (existing.empty()) {
        mb->addMenu(applicationMenu);
        if (toolsMenu) mb->addMenu(toolsMenu);
    } else {
        mb->insertMenu(existing.front(), applicationMenu);
        if (toolsMenu) mb->insertMenu(existing.front(), toolsMenu);
    }

    mb->addMenu(windowMenu_);
    mb->addMenu(loggingMenu);
    mb->addMenu(helpMenu);
}

void
MainWindowBase::fillContextMenu(QMenu* menu)
{
    menu->addActions(menuBar()->actions());
}

QAction*
MainWindowBase::getShowAction()
{
    if (!showAction_) showAction_ = makeShowAction();
    return showAction_;
}

void
MainWindowBase::saveToSettings(QSettings& settings)
{
    static Logger::ProcLog log("saveToSettings", Log());
    LOGINFO << settings.group() << std::endl;
    settings.setValue("Geometry", saveGeometry());
    settings.setValue("State", saveState());
}

void
MainWindowBase::restoreFromSettings(QSettings& settings)
{
    static Logger::ProcLog log("restoreFromSettings", Log());
    LOGINFO << settings.group() << std::endl;
    restoreGeometry(settings.value("Geometry").toByteArray());
    restoreState(settings.value("State").toByteArray());
}

void
MainWindowBase::setFixedSize()
{
    statusBar()->setSizeGripEnabled(false);
    if (layout()) layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void
MainWindowBase::contextMenuEvent(QContextMenuEvent* event)
{
    if (contextMenu_) contextMenu_->exec(event->globalPos());
}

void
MainWindowBase::changeEvent(QEvent* event)
{
    static Logger::ProcLog log("changeEvent", Log());
    LOGINFO << "type: " << event->type() << std::endl;
    LOGINFO << "windowState: " << std::hex << int(windowState()) << std::dec << std::endl;

    Super::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent* tmp = static_cast<QWindowStateChangeEvent*>(event);

        bool wasFullScreen = tmp->oldState() & Qt::WindowFullScreen;
        bool nowFullScreen = windowState() & Qt::WindowFullScreen;

        if (wasFullScreen != nowFullScreen) {
            LOGDEBUG << "change in fullscreen state" << std::endl;
            if (nowFullScreen) {
                LOGDEBUG << "entered fullscreen state" << std::endl;
                menuBar()->hide();
                window()->activateWindow();
            } else {
                LOGDEBUG << "exited fullscreen state" << std::endl;
                menuBar()->show();
                window()->activateWindow();
            }
        }
    }
}

void
MainWindowBase::showEvent(QShowEvent* event)
{
    Logger::ProcLog log("showEvent", Log());
    LOGINFO << std::endl;
    if (!initialized_) initialize();
    getApp()->manageWindow(this, getWindowMenu());
    Super::showEvent(event);
}

bool
MainWindowBase::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        if (restorable_) {
            QSettings settings;
            settings.beginGroup("MainWindows");
            settings.beginGroup(objectName());
            restoreFromSettings(settings);
            if (visibleAfterRestore_) showAndRaise();
        }
    }

    return Super::event(event);
}

void
MainWindowBase::closeEvent(QCloseEvent* event)
{
    Logger::ProcLog log("closeEvent", Log());

    if (restorable_) {
        QSettings settings;
        settings.beginGroup("MainWindows");
        settings.beginGroup(objectName());
        saveToSettings(settings);
    }

    AppBase* app = getApp();
    if (app->getNumManagedWindows() == 1 && !app->isQuitting()) {
        event->ignore();
        QTimer::singleShot(0, app, SLOT(applicationQuit()));
        return;
    }

    getApp()->unmanageWindow(this);
    Super::closeEvent(event);
}

QAction*
MainWindowBase::makeMenuAction(const QString& title, const char* slot, int shortcut)
{
    return MakeMenuAction(title, this, slot, shortcut);
}

QAction*
MainWindowBase::makeShowAction(int shortcut)
{
    return makeMenuAction(windowTitle(), SLOT(showAndRaise()), shortcut);
}

void
MainWindowBase::setWindowTitle(const QString& title)
{
    Super::setWindowTitle(title);
    if (showAction_) showAction_->setText(title);
}

ToolBar*
MainWindowBase::makeToolBar(const QString& title, Qt::ToolBarArea area)
{
    ToolBar* toolBar = new ToolBar(title, toolBarMenu_, this);

    // Give the new toolbar a name based on its title -- just remove any spaces in the title.
    //
    toolBar->setObjectName(title.split(' ', QString::SkipEmptyParts).join(""));

    // Set the initial location of the toolbar. May be changed by the user.
    //
    addToolBar(area, toolBar);

    // Get the action that controls toolbar visibility, and add to the window's toolbar menu.
    //
    QAction* action = toolBar->toggleViewAction();

    // This only affects applications running on MacOS X. Disable Qt's attempt to rearrange menu items to
    // conform to Apple's guidelines.
    //
    action->setMenuRole(QAction::NoRole);

    toolBarMenu_->addAction(action);

    return toolBar;
}

void
MainWindowBase::showAndRaise()
{
    if (isMinimized())
        showNormal();
    else
        show();
    raise();
    activateWindow();
}
