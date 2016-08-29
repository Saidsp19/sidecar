#ifndef SIDECAR_GUI_MAINWINDOWBASE_H // -*- C++ -*-
#define SIDECAR_GUI_MAINWINDOWBASE_H

#include "QtGui/QMainWindow"

class QAction;
class QMenu;
class QSettings;

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class AppBase;
class ToolBar;

/** Common base class for all SideCar GUI projects. Holds features and functionality shared among all of the GUI
    applications. Saves and restores the window position and size between application runs. Manages window
    minimization, mazimization, and full-screen mode using a common window menu, managed by the application's
    WindowManager object.
*/
class MainWindowBase : public QMainWindow
{
    Q_OBJECT
    using Super = QMainWindow;
public:

    /** Log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.
     */
    MainWindowBase();

    /** Destructor.
     */
    ~MainWindowBase();

    /** Complete widget initialization. As this is a virtual method, derived classes should override this to
	implement their custom initialization. The constructor is not a good place for some initialization
	because virtual methods are not honored there.
    */
    virtual void initialize();

    /** Obtain the QAction object that toggles the tool window's visibility. If one does not yet exist, it will
        invoke makeShowAction() to create one.

        \return QAction object
    */
    QAction* getShowAction();

    /** Configure the window so that the user may not resize it.
     */
    void setFixedSize();
    
    /** Obtain the application object.

        \return AppBase object
    */
    static AppBase* getApp();

    /** Save window settings in a QSettings object

        \param settings container to hold the window settings
    */
    virtual void saveToSettings(QSettings& settings);

    /** Restore a window configuration using settings from a QSettings object.

        \param settings container with the window settings to use
    */
    virtual void restoreFromSettings(QSettings& settings);

    /** Shadow of QWidget::setWindowTitle() method. Informs the AppBase object managing the window of the title
        change.

        \param windowTitle 
    */
    void setWindowTitle(const QString& windowTitle);

    /** Obtain the 'Window' menu for this window. The 'Window' menu contains window-specific actions such as
        'Maximize' and 'Minimize', and it also maintains a list of open windows for easy selection by the user.

        \return QMenu object
    */
    QMenu* getWindowMenu() const { return windowMenu_; }

    /** Obtain the menu item that holds QAction items that control the visibility of toolbars registered with
        makeToolBar().

        \return QMenu object
    */
    QMenu* getToolBarMenu() const { return toolBarMenu_; }

    /** Notification from the application's WindowManager that the 'Window' menu is about to show itself.
        Derived classes should override this method if they need to update one or more QAction objects before
        the menu is displayed.

        \param actions list of QAction objects that make up the 'Window' menu.
    */
    virtual void windowMenuAboutToShow(QList<QAction*>& actions) {}

    /** If a window is restorable, then the active WindowManager object will call its saveToSettings() method
        when the application quits. When the application launches again, it will recreate the saved
        MainWindowBase objects, restoring them to their original size and location.

        \param state new state to use
    */
    void setRestorable(bool state)
	{ restorable_ = state; }

    /** Determine of the window is restorable.

        \return true if so
    */
    bool isRestorable() const { return restorable_; }

    /** If a window is restorable, this flag controls whether is it also initially visible to the user when it
        is restored.

        \param state new state to use
    */
    void setVisibleAfterRestore(bool state)
	{ visibleAfterRestore_ = state; }

    /** Determine if the window should be visible when the application relaunches.

        \return true if so
    */
    bool isVisibleAfterRestore() const { return visibleAfterRestore_; }

public slots:

    /** Convenience slot method that shows our window, raises it to the top of the window layer, and then
	activates it.
    */
    virtual void showAndRaise();

protected:

    /** Install a QMenu object to use for the window's 'Window' menu. Should be invoked prior to initialize().

        \param menu the QMenu to use
    */
    void setWindowMenu(QMenu* menu) { windowMenu_ = menu; }

    /** Install a QMenu object to use for the window's context menu (the menu that pops up when the user
	right-clicks (or CTRL+click on Mac) inside the window.

        \param menu the QMenu to use
    */
    void setContextMenu(QMenu* menu) { contextMenu_ = menu; }

    /** Create the window's QMenuBar object. Populates it with QMenu objects. This implementation creates or
	uses the following menus:

	- Tools
	- Window
	- Logging
	- Help
    */
    virtual void makeMenuBar();

    /** Add QAction objects to the QMenu object used as a context menu. Invoked from within the initialize()
        method.

        \param menu the QMenu object to work with
    */
    virtual void fillContextMenu(QMenu* menu);

    /** Create a new QAction that controls our visibility. Invoked from getShowAction(). The default behavior is
	to create a new QAction with no key shortcut. Derived classes can override this, and provide a different
	shortcut value.

	\param shortcut key shortcut to use for the QAction

	\return new QAction object
    */
    virtual QAction* makeShowAction(int shortcut = 0);

    /** Create a new QAction object, with this as the parent of the new object and the receiver of the action's
        triggered() signal.

        \param title name of the new action

        \param slot the slot to connect to the action's triggered() signal.

        \param shortcut key sequence associated with the action

        \return new QAction object
    */
    QAction* makeMenuAction(const QString& title, const char* slot = 0,
                            int shortcut = 0);

    /** Create a new QToolBar object with a given title and initial position. Add a visibility QAction object to
        the internal toolbar menu to control toolbar visibility.

        \param title name of the new toolbar, as shown in the toolbar menu

        \param area default location for the toolbar

        \return new QToolBar object
    */
    virtual ToolBar* makeToolBar(const QString& title,
                                 Qt::ToolBarArea area = Qt::TopToolBarArea);

    bool event(QEvent* event);
    
    /** Called in response to a user request to show a context menu. Displays one if a derived class overrides
        getContextMenu() to return a non-NULL value. Override of QWidget method.

	\param event event descriptor
    */
    void contextMenuEvent(QContextMenuEvent* event);

    /** Called just before the window is shown to the user. Calls AppBase::manageWindow() to tell the
        application manage our window with its Window menu. Override of QWidget method.

        \param event event descriptor
    */
    void showEvent(QShowEvent* event);

    /** Called when the widget has been closed, either programmatically or due to user action. Removes the
        window from management by AppBase. Overrides of QWidget method.

        \param event event descriptor
    */
    void closeEvent(QCloseEvent* event);

    /** Looks for QEvent::WindowStateChange events to detect when entering/exiting full-screen mode in order to
        hide/show the menu and status bars. Override of QWidget method.

	\param event event descriptor
    */
    void changeEvent(QEvent* event);

private:
    QMenu* windowMenu_;
    QMenu* contextMenu_;
    QMenu* toolBarMenu_;
    QAction* showAction_;
    bool initialized_;
    bool restorable_;
    bool visibleAfterRestore_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
