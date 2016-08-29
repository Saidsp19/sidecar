#include "QtCore/QSettings"
#include "QtGui/QAction"
#include "QtGui/QApplication"
#include "QtGui/QMenu"
#include "QtGui/QStatusBar"

#include "AppBase.h"
#include "LogUtils.h"
#include "MainWindowBase.h"
#include "ToolBar.h"
#include "Utils.h"
#include "WindowManager.h"

using namespace SideCar::GUI;

Logger::Log&
WindowManager::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.WindowManager");
    return log_;
}

WindowManager::WindowManager(QObject* parent)
    : QObject(parent), managedWindows_(), managedWindowMenus_(),
      managedWindowRaiseActions_(), activeMainWindow_(0)
{
    makeWindowMenuActions();
}

void
WindowManager::addWindowMenuActionTo(ToolBar* toolBar, int index)
{
    toolBar->addAction(windowMenuActions_[index]);
}

QMenu*
WindowManager::makeWindowMenu(QWidget* parent)
{
    static Logger::ProcLog log("makeWindowMenu", Log());

    // Create a new Window menu, but do not populate it yet with QAction objects; wait until a call to
    // managedWindow() to do that.
    //
    QMenu* menu = new QMenu("Window", parent);
    connect(menu, SIGNAL(aboutToShow()), SLOT(windowMenuAboutToShow()));
    LOGDEBUG << "makeWindowMenu: " << menu << std::endl;
    return menu;
}

void
WindowManager::makeWindowMenuActions()
{
    Logger::ProcLog log("makeWindowMenuActions", Log());
    LOGINFO << std::endl;

    // Create all actions that may be present in the Windows menu.
    //
    windowMenuActions_.push_back(
	MakeMenuAction("New", AppBase::GetApp(), SLOT(windowNew()),
                       Qt::CTRL + Qt::Key_N));
    windowMenuActions_.push_back(
	MakeMenuAction("Close", this, SLOT(windowClose()),
                       Qt::CTRL + Qt::Key_W));

    // The following separator will disappear if neither of the above actions is shown.
    //
    windowMenuActions_.push_back(new QAction(this));
    windowMenuActions_.back()->setSeparator(true);

    windowMenuActions_.push_back(
	MakeMenuAction("Minimize", this, SLOT(windowMinimize()),
                       Qt::CTRL + Qt::Key_M));
    windowMenuActions_.push_back(
	MakeMenuAction("Maximize", this, SLOT(windowMaximize()),
                       Qt::CTRL + Qt::SHIFT + Qt::Key_M));
    QAction* action = windowMenuActions_.back();
    action->setData(QStringList() << "Maximize" << "Unmaximize");

    windowMenuActions_.push_back(
	MakeMenuAction("Enter Full-Screen Mode", this,
                       SLOT(windowFullScreen()), Qt::CTRL + Qt::Key_F));
    action = windowMenuActions_.back();
    action->setToolTip("Enter full-screen mode.");
    action->setIcon(QIcon(":/fullscreen.png"));
    action->setData(QStringList() << "Enter" << "Leave");
    action->setCheckable(true);

    // Create a separator between the above actions and those for managed windows.
    //
    windowMenuActions_.push_back(new QAction(this));
    windowMenuActions_.back()->setSeparator(true);

    windowMenuActions_.push_back(new QAction("Tool Bars", this));
    windowMenuActions_.back()->setEnabled(true);

    windowMenuActions_.push_back(new QAction(this));
    windowMenuActions_.back()->setSeparator(true);

    Q_ASSERT(windowMenuActions_.size() == kNumWindowMenuActions);
}

QWidget*
WindowManager::getManagedWindow(const QString& title) const
{
    for (int index = 0; index < managedWindows_.size(); ++index) {
	if (title == managedWindows_[index]->windowTitle())
	    return managedWindows_[index];
    }

    return 0;
}

QString
WindowManager::windowTitleToActionText(QString title, bool changed)
{
    int pos = title.indexOf("[*]");
    if (pos != -1)
	title.remove(pos, 3);
    if (changed) {
	title += ' ';
	title += QChar(0x00A4);
    }
    return title;
}

void
WindowManager::windowTitleChanged(QWidget* window)
{
    Logger::ProcLog log("windowTitleChanged", Log());
    LOGINFO << "window: " << window << " new title: " << window->windowTitle()
	    << std::endl;
    int index = managedWindows_.indexOf(window);
    if (index == -1) return;
    LOGDEBUG << "index: " << index << " old title: "
	     << managedWindowRaiseActions_[index]->text() << std::endl;
    managedWindowRaiseActions_[index]->setText(
	windowTitleToActionText(window->windowTitle(),
                                window->isWindowModified()));
}

bool
WindowManager::eventFilter(QObject* obj, QEvent* event)
{
    static Logger::ProcLog log("eventFilter", Log());

    int index = managedWindows_.indexOf(static_cast<QWidget*>(obj));
    if (index == -1) return Super::eventFilter(obj, event);

    QWidget* widget = managedWindows_[index];

    if (event->type() == QEvent::ActivationChange) {
	LOGDEBUG << "window: " << widget->windowTitle()
		 << " activation changed - isActive: "
		 << widget->isActiveWindow() << std::endl;
	if (widget->isActiveWindow()) {
	    setActiveWindow(widget);
	}
    }
    else if (event->type() == QEvent::WindowTitleChange) {
	LOGDEBUG << "window: " << widget->windowTitle() << " windowTitleChange"
		 << std::endl;
	windowTitleChanged(widget);
    }

    return false;
}

void
WindowManager::saveWindows()
{
    Logger::ProcLog log("saveWindows", Log());
    QSettings settings;
    int counter = 0;
    settings.beginWriteArray("Windows");
    for (int index = 0; index < managedWindows_.size(); ++index) {
	QWidget* window = managedWindows_[index];
	MainWindowBase* mainWindow = qobject_cast<MainWindowBase*>(window);
	if (mainWindow && mainWindow->isRestorable()) {
	    settings.setArrayIndex(counter++);
	    LOGDEBUG << "saving " << window->windowTitle() << std::endl;
	    settings.setValue("ObjectName", window->objectName());
	}
    }
    settings.endArray();

    LOGDEBUG << "saved " << counter << " windows" << std::endl;
}

void
WindowManager::manageWindow(QWidget* window, QMenu* windowMenu)
{
    Logger::ProcLog log("manageWindow", Log());
    LOGINFO << window << ' ' << window->windowTitle() << std::endl;

    if (managedWindows_.indexOf(window) != -1) {
	LOGDEBUG << window->windowTitle() << " is already managed"
		 << std::endl;
	return;
    }

    // Just to be safe, remove ourselves from the window's list of event filters. No problem if we are not
    // present.
    //
    window->removeEventFilter(this);
    window->installEventFilter(this);

    // For the first 9 windows, provide a numeric key shortcut
    //
    int shortcut = 0;
    if (managedWindows_.size() < 9) {
	shortcut = Qt::CTRL + Qt::ALT + Qt::Key_1 + managedWindows_.size();
	LOGDEBUG << "assigning shortcut " << shortcut << std::endl;
    }

    // Fetch the window's Window menu, and recreate it to match the current window environment.
    //
    if (windowMenu) {
	windowMenu->clear();
	windowMenu->addActions(windowMenuActions_);
	windowMenu->addActions(managedWindowRaiseActions_);
    }

    managedWindows_.push_back(window);
    managedWindowMenus_.push_back(windowMenu);

    // Create a new QAction object that will raise and activate the managed window.
    //
    QAction* action = new QAction(
	windowTitleToActionText(window->windowTitle(),
                                window->isWindowModified()), this);
    action->setData(QVariant::fromValue(window));
    LOGDEBUG << "action: " << action << std::endl;

    managedWindowRaiseActions_.push_back(action);
    action->setCheckable(true);
    // action->setShortcutContext(Qt::ApplicationShortcut);
    if (shortcut)
	action->setShortcut(shortcut);

    connect(action, SIGNAL(triggered()), SLOT(windowRaiseAndActivate()));

    // Add the action to the all managed Window menus
    //
    for (int index = 0; index < managedWindowMenus_.size(); ++index)
	if (managedWindowMenus_[index])
	    managedWindowMenus_[index]->addAction(action);

    LOGDEBUG << "managedWindows_.size: " << managedWindows_.size()
	     << " managedWindowRaiseActions_.size: "
	     << managedWindowRaiseActions_.size()
	     << std::endl;
}

void
WindowManager::windowRaiseAndActivate()
{
    QAction* action = qobject_cast<QAction*>(sender());
    int index = managedWindowRaiseActions_.indexOf(action);
    if (index != -1) {
	QWidget* window = managedWindows_[index];
	window->raise();
	window->activateWindow();
    }
}

void
WindowManager::unmanageWindow(QWidget* window)
{
    Logger::ProcLog log("unmanageMainWindow", Log());
    LOGINFO << window << ' ' << window->windowTitle() << std::endl;

    if (window == activeMainWindow_) {
	LOGDEBUG << "was active window" << std::endl;
	setActiveMainWindow(0);
    }

    int index = managedWindows_.indexOf(window);
    if (index == -1) return;

    LOGDEBUG << "index: " << index << std::endl;

    LOGDEBUG << "managedWindows_.size: " << managedWindows_.size()
	     << " managedWindowRaiseActions_.size: "
	     << managedWindowRaiseActions_.size()
	     << std::endl;

    window->removeEventFilter(this);

    // Remove the managed items from their containers. We only delete the QAction object since that was created
    // in a previous managedWindow() call. Use deleteLater() so there is no chance of deleting it while it is in
    // use by Qt.
    //
    managedWindowMenus_.removeAt(index);
    managedWindows_.removeAt(index);
    LOGDEBUG << "OK managedWindows_.removeAt() " << std::endl;
    QAction* action = managedWindowRaiseActions_.takeAt(index);
    LOGDEBUG << "OK managedWindowRaiseActions_.takeAt: " << action << std::endl;
    action->deleteLater();

    // Renumber the remaining actions. NOTE: changes the value of index
    //
    for (; index < managedWindowRaiseActions_.size() && index < 9; ++index) {
	managedWindowRaiseActions_[index]->setShortcut(
	    Qt::CTRL + Qt::ALT + Qt::Key_1 + index);
    }

    // Give focus to the next window in the managed order.
    //
    if (! managedWindows_.empty()) {
	window = managedWindows_.back();
	window->raise();
	window->activateWindow();
    }

    LOGDEBUG << "finished" << std::endl;
}

void
WindowManager::setActiveWindow(QWidget* window)
{
    Logger::ProcLog log("setActiveWindow", Log());
    LOGINFO << window << ' ' << window->windowTitle() << std::endl;

    // See if the top-level window of the widget with focus is a managed window. If so, and it is not the
    // current active window, notify it that it is now the active one.
    //
    LOGDEBUG << "new active main window: " << window << std::endl;
    LOGDEBUG << "title: " << window->windowTitle() << std::endl;
    int index = managedWindows_.indexOf(window);
    if (index == -1) {
	LOGERROR << "called with unmanaged window" << std::endl;
	return;
    }

    // Reorder the managed containers so that they reflect the window order on the screen.
    //
    managedWindows_.push_back(managedWindows_.takeAt(index));
    managedWindowMenus_.push_back(managedWindowMenus_.takeAt(index));
    managedWindowRaiseActions_.push_back(
	managedWindowRaiseActions_.takeAt(index));
    for (int index = 0; index < managedWindowRaiseActions_.size() - 1;
         ++index) {
	managedWindowRaiseActions_[index]->setChecked(false);
    }

    managedWindowRaiseActions_.back()->setChecked(true);

    MainWindowBase* mainWindow = qobject_cast<MainWindowBase*>(window);
    if (mainWindow)
	setActiveMainWindow(mainWindow);
}

void
WindowManager::setActiveMainWindow(MainWindowBase* window)
{
    Logger::ProcLog log("setActiveMainWindow", Log());
    LOGINFO << window << std::endl;
    if (window != activeMainWindow_) {
	activeMainWindow_ = window;
	if (window) 
	    emit activeMainWindowChanged(window);
    }
}

void
WindowManager::windowMenuAboutToShow()
{
    Logger::ProcLog log("windowMenuAboutToShow", Log());
    LOGINFO << "activeMainWindow: " << activeMainWindow_ << std::endl;

    QWidget* active = AppBase::GetApp()->activeWindow();
    MainWindowBase* main = getActiveMainWindow();
    LOGDEBUG << "main: " << main << std::endl;
    if (main) {
	QMenu* menu = main->getToolBarMenu();
	LOGDEBUG << "menu: " << menu << ' ' << menu->actions().size()
		 << std::endl;

	if (windowMenuActions_[kToolBarMenu]->menu() != menu) {
	    windowMenuActions_[kToolBarMenu]->setMenu(menu);
	    windowMenuActions_[kToolBarMenu]->setEnabled(
		! menu->actions().empty());
	}
	else if (! menu) {
	    windowMenuActions_[kToolBarMenu]->setEnabled(false);
	}	    

	main->windowMenuAboutToShow(windowMenuActions_);
    }
    else {
	windowMenuActions_[kNew]->setEnabled(false);
	windowMenuActions_[kToolBarMenu]->setEnabled(false);
	LOGDEBUG << "disabled tool bar menu" << std::endl;
    }

    windowMenuActions_[kClose]->setEnabled(active);
    windowMenuActions_[kMinimize]->setEnabled(active);
    windowMenuActions_[kMaximize]->setEnabled(active);
    windowMenuActions_[kFullScreen]->setEnabled(active);

    // Visibility of the first separator depends on "New" or "Close" being visible.
    //
    windowMenuActions_[kSeparator1]->setVisible(true ||
                                                windowMenuActions_[kNew]->isVisible() ||
                                                windowMenuActions_[kClose]->isVisible());

    if (active) {
	windowMenuActions_[kMinimize]->setText(
	    active->isMinimized() ? "Unminimize" : "Minimize");
	windowMenuActions_[kMaximize]->setText(
	    active->isMaximized() ? "Unmaximize" : "Maximize");
	windowMenuActions_[kFullScreen]->setEnabled(active == main);
	windowMenuActions_[kFullScreen]->setText(
	    active->isFullScreen() ? "Leave Full Screen Mode" :
	    "Enter Full Screen Mode");
    }	
    else {
	windowMenuActions_[kNew]->setEnabled(false);
    }

    for (int index = 0; index < managedWindowRaiseActions_.size(); ++index) {
	bool modified = managedWindows_[index]->isWindowModified();
	managedWindowRaiseActions_[index]->setText(
	    windowTitleToActionText(managedWindows_[index]->windowTitle(),
                                    modified));
    }
}

void
WindowManager::windowClose()
{
    QWidget* window = AppBase::GetApp()->activeWindow();
    if (! window) return;
    window->close();
}

void
WindowManager::windowMinimize()
{
    QWidget* window = AppBase::GetApp()->activeWindow();
    if (! window) return;
    if (window->isMinimized()) {
	window->showNormal();
    }
    else {
	window->showMinimized();
    }
}

void
WindowManager::windowMaximize()
{
    QWidget* window = AppBase::GetApp()->activeWindow();
    if (! window) return;
    if (window->isMaximized()) {
	window->showNormal();
    }
    else {
	window->showMaximized();
    }
}

void
WindowManager::windowFullScreen()
{
    QWidget* window = AppBase::GetApp()->activeWindow();
    if (! window) return;
 
    bool isFullScreen = window->isFullScreen();
    QAction* action = windowMenuActions_[kFullScreen];
    UpdateToggleAction(action, ! isFullScreen);

    if (isFullScreen) {
	window->showNormal();
	action->setText("Enter Full Screen Mode");
    }
    else {
	window->showFullScreen();
	action->setText("Leave Full Screen Mode");
    }
}

void
WindowManager::showMessage(const QString& msg, int duration)
{
    if (activeMainWindow_)
	activeMainWindow_->statusBar()->showMessage(msg, duration);
}

void
WindowManager::showOrHideMenuAction(QAction* action, bool state)
{
    action->setEnabled(state);
    action->setVisible(state);
}
