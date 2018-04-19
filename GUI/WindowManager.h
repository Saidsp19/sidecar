#ifndef SIDECAR_GUI_WINDOWMANAGER_H // -*- C++ -*-
#define SIDECAR_GUI_WINDOWMANAGER_H

#include "QtCore/QList"
#include "QtWidgets/QAction"
#include "QtWidgets/QWidget"

class QMenu;

namespace Logger {
class ConfiguratorFile;
class Log;
} // namespace Logger

namespace SideCar {
namespace GUI {

class MainWindowBase;
class ToolBar;

/** Manager of an application's 'Window' menu. The 'Window' menu contains QAction objects that affect the size
    and visibility of MainWindowBase objects. It also contains a list of the active MainWindowBase objects, and
    automatically manages shortcut keys for them. Finally, the 'Window' menu contains a submenu that lists all
    of the active ToolBar objects.
*/
class WindowManager : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    /** Enumeration for the various window actions found in the Window menu.
     */
    enum WindowMenuAction {
        kNew = 0,
        kClose,
        kSeparator1,
        kMinimize,
        kMaximize,
        kFullScreen,
        kSeparator2,
        kToolBarMenu,
        kSeparator3,
        kNumWindowMenuActions
    };

    /** Log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor. There should be only one WindowManager object for an application, but this is not enforced.

        \param parent the application owner of this object
    */
    WindowManager(QObject* parent = 0);

    /** Create the 'Window' menu for a given widget. The given widget is usually derived from MainWindowBase,
        but that is not actually necessary.

        \param parent the widget to use for ownership

        \return new QMenu object for the widget
    */
    QMenu* makeWindowMenu(QWidget* parent);

    /** Obtain the QAction object found within the shared 'Window' menu. These are the WindowMenuAction commands
        listed above.

        \param index which action to obtain

        \return QAction object
    */
    void addWindowMenuActionTo(ToolBar* toolBar, int index);

    /** Set the visibility state of the "New" action in the "Window" menu.

        \param visible true if visible
    */
    void setVisibleWindowMenuNew(bool visible) { showOrHideMenuAction(windowMenuActions_[kNew], visible); }

    /** Set the visibility state of the "Close" action in the "Window" menu.

        \param visible true if visible
    */
    void setVisibleWindowMenuClose(bool visible) { showOrHideMenuAction(windowMenuActions_[kClose], visible); }

    /** Set the visibility state of the "Minimize" action in the "Window" menu.

        \param visible true if visible
    */
    void setVisibleWindowMenuMinimize(bool visible) { showOrHideMenuAction(windowMenuActions_[kMinimize], visible); }

    /** Set the visibility state of the "Maximize" action in the "Window" menu.

        \param visible true if visible
    */
    void setVisibleWindowMenuMaximize(bool visible) { showOrHideMenuAction(windowMenuActions_[kMaximize], visible); }

    /** Set the visibility state of the full-screen action in the "Window" menu.

        \param visible true if visible
    */
    void setVisibleWindowMenuFullScreen(bool visible)
    {
        showOrHideMenuAction(windowMenuActions_[kFullScreen], visible);
    }

    /** Obtain the number of open MainWindowBase objects.

        \return window count
    */
    int getNumManagedWindows() const { return managedWindows_.size(); }

    /** Obtain the active MainWindowBase object. NOTE: this may be NULL.

        \return active MainWindowBase object
    */
    MainWindowBase* getActiveMainWindow() const { return activeMainWindow_; }

    /** Manage a top-level window. Managed windows have their own QAction object installed in the "Window" menu
        so that the user can make a window active from the menubar or keyboard.

        \param window the window to manage
    */
    void manageWindow(QWidget* window, QMenu* windowMenu);

    /** Unmanage a top-level window. Usually invoked when a window closes. Removes the associated QAction object
        from the "Window" menu.

        \param window the window to unmanage.
    */
    void unmanageWindow(QWidget* window);

    /** Determine if a given widget is managed by the application.

        \param window the widget to test

        \return true if so
    */
    bool isManagedWindow(QWidget* window) const { return managedWindows_.indexOf(window) != -1; }

    /** Obtain the QWidget with a given name.

        \param title

        \return
    */
    QWidget* getManagedWindow(const QString& title) const;

    void saveWindows();

signals:

    void activeMainWindowChanged(MainWindowBase* window);

public slots:

    /** Show a message in the active main window.

        \param msg text of the message to show

        \param duration amount of time in milliseconds to show the message;
        zero shows the message forever until cleared or replaced by another
        message.
    */
    void showMessage(const QString& msg, int duration);

private slots:

    void windowMenuAboutToShow();

    /** Close the active main window. Does not affect tool windows.
     */
    void windowClose();

    /** Toggle the minimize state of the active main window. Does not affect tool windows.
     */
    void windowMinimize();

    /** Toggle the maximize state of the active main window. Does not affect tool windows.
     */
    void windowMaximize();

    /** Toggle the full-screen state of the active window. In full-screen mode, the active window's content
        fills the entire screen, with no menubar or status bar. Does not affect tool windows.
    */
    void windowFullScreen();

    void windowRaiseAndActivate();

private:
    void makeWindowMenuActions();

    void showOrHideMenuAction(QAction* action, bool state);

    void windowTitleChanged(QWidget* widget);

    bool eventFilter(QObject* obj, QEvent* event);

    void setActiveWindow(QWidget* widget);

    void setActiveMainWindow(MainWindowBase* window);

    QString windowTitleToActionText(QString title, bool changed = false);

    QList<QAction*> windowMenuActions_;
    QList<QWidget*> managedWindows_;
    QList<QMenu*> managedWindowMenus_;
    QList<QAction*> managedWindowRaiseActions_;
    MainWindowBase* activeMainWindow_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
