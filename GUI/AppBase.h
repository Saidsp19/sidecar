#ifndef SIDECAR_GUI_APPBASE_H // -*- C++ -*-
#define SIDECAR_GUI_APPBASE_H

#include <vector>

#include "QtCore/QDir"
// #include "QtGui/QAction"
#include "QtWidgets/QApplication"

class QAction;
class QMenu;
class QWidget;

namespace Logger {
class ConfiguratorFile;
class Log;
} // namespace Logger

namespace SideCar {
namespace GUI {

class DisableAppNap;
class LoggerWindow;
class MainWindowBase;
class ManualWindow;
class ToolBar;
class ToolWindowBase;
class WindowManager;

/** Base application class used by all GUI applications. Provides window management actions, and logging menu
    actions. Also provides saving/restoring facilities so that restarted applications have windows and settings
    from a previous run.
*/
class AppBase : public QApplication {
    Q_OBJECT
    using Super = QApplication;

public:
    /** Enumeration for the various angular formatting available from the application.
     */
    enum AngularFormatType { kDegreesMinutesSeconds = 0, kDecimal, kNumFormattingTypes };

    /** Log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Obtain the AppBase singleton.

        \return AppBase object
    */
    static AppBase* GetApp();

    /** Constructor.

        \param name the name of the application

        \param argc argument count from the command line

        \param argv argument values from the command line
    */
    AppBase(const QString& name, int& argc, char** argv);

    /** Destructor.
     */
    ~AppBase();

    const QString& getApplicationName() const { return name_; }

    WindowManager* getWindowManager() const { return windowManager_; }

    QMenu* makeApplicationMenu(QWidget* parent);

    QMenu* makeToolsMenu(QWidget* parent);

    QMenu* makeWindowMenu(QWidget* parent);

    QMenu* makeLoggingMenu(QWidget* parent);

    QMenu* makeHelpMenu(QWidget* parent);

    /** Obtain a specific QAction object found in the Tools menu.

        \param index which QAction to fetch

        \return QAction object
    */
    QAction* getToolsMenuAction(int index) { return toolsMenuActions_[index]; }

    void addWindowMenuActionTo(ToolBar* toolBar, int index) const;

    /** Set the visibility state of the "New" action in the "Window" menu.

        \param visible true if visible
    */
    void setVisibleWindowMenuNew(bool visible);

    /** Set the visibility state of the "Close" action in the "Window" menu.

        \param visible true if visible
    */
    void setVisibleWindowMenuClose(bool visible);

    /** Set the visibility state of the "Minimize" action in the "Window" menu.

        \param visible true if visible
    */
    void setVisibleWindowMenuMinimize(bool visible);

    /** Set the visibility state of the "Maximize" action in the "Window" menu.

        \param visible true if visible
    */
    void setVisibleWindowMenuMaximize(bool visible);

    /** Set the visibility state of the full-screen action in the "Window" menu.

        \param visible true if visible
    */
    void setVisibleWindowMenuFullScreen(bool visible);

    /** Use a configuration file for the Logger classes.

        \param path file path
    */
    void setLogConfigurationFile(const QString& path);

    /** Obtain the number of open MainWindowBase objects.

        \return window count
    */
    int getNumManagedWindows() const;

    /** Obtain the active MainWindowBase object. NOTE: this may be NULL.

        \return active MainWindowBase object
    */
    MainWindowBase* getActiveMainWindow() const;

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
    bool isManagedWindow(QWidget* window) const;

    QWidget* getManagedWindow(const QString& title) const;

    /** Determine if the application is in the process of shutting down. This flag is only set from within the
        applicationQuit() slot.

        \return true if so
    */
    bool isQuitting() const { return quitting_; }

    /** Obtain the current distance units suffix. Note that the suffix already has a space at the start.

        \return current distance units.
    */
    const QString& getDistanceUnits() const { return distanceUnits_; }

    /** Obtain a formatted representation of a distance value.

        \param value the numerical value to format

        \return textual distance value
    */
    QString getFormattedDistance(double value) const;

    /** Obtain a formatted representation of an angular value in radians.

        \param value the numerical value in radians to format

        \return textual angular value
    */
    QString getFormattedAngleRadians(double value) const;

    /** Obtain a formatted representation of an angular value in degrees.

        \param value the numerical value in degrees to format

        \return textual angular value
    */
    QString getFormattedAngleDegrees(double value) const;

    /** Obtain the current phantom cursor value. The AppBase object only updates this value from the
        setPhantomCursor() slot.

        \return QPointF value
    */
    QPointF getPhantomCursor() const { return phantomCursor_; }

    /** Restore the windows that were open at the time the application last quit. Emits the restoreToolWindows()
        signal after all main windows have been restored.
    */
    virtual void restoreWindows();

signals:

    /** Notification sent out when the application's active main window changes.

        \param window the new active window (may be NULL).
    */
    void activeMainWindowChanged(MainWindowBase* window);

    /** Notification sent out when the application is shutting down.
     */
    void shutdown();

    /** Notification sent out to all ToolWindowBase objects that they should show themselves if they were
        visible when the application last quit.
    */
    void restoreToolWindows();

    /** Notification sent out when the angular formatting setting has been changed by the user.

        \param type the new formatting type in use
    */
    void angleFormattingChanged(int type);

    /** Notification sent out when the distance units have been changed by the user.

        \param suffix new distance units
    */
    void distanceUnitsChanged(const QString& suffix);

    /** Notification sent out when the phantom cursor value has been changed.

        \param pos new phantom cursor value
    */
    void phantomCursorChanged(const QPointF& pos);

public slots:

    void updateToggleAction(bool state);

    /** Set the held phantom cursor value. Emits the phantomCursorChanged() signal if the given value is
        different than the held one.

        \param pos new phantom cursor value
    */
    virtual void setPhantomCursor(const QPointF& pos);

    /** Show a message in the active main window.

        \param msg text of the message to show

        \param duration amount of time in milliseconds to show the message; zero shows the message forever until
        cleared or replaced by another message.
    */
    void showMessage(const QString& msg, int duration);

    /** Change the formatting used by getFormattedAngleDegrees() and getFormattedAngleRadians() methods.

        \param type new type to use
    */
    void setAngleFormatting(AngularFormatType type);

    /** Change the distance units used by getFormattedDistance() method.

        \param units new units to use
    */
    void setDistanceUnits(const QString& units);

    /** Action event handler for the "Quit" menu item. Saves open window settings, the current logging settings,
        emits the shutdown() signal, and then finally closes all open windows.
    */
    virtual void applicationQuit();

    /** Present to the user an open file dialog in order to select a Logger configuration file. If a file is
        choosen, create and install a Logger::ConfiguratorFile object.
    */
    virtual void loggingLoadConfigurationFile();

    virtual void loadStyleSheet();

    virtual void showAbout();

    virtual void windowNew();

protected:
    /** Register a tool window with the application. Adds the tool window's show/hide QAction object to the end
        of the Application menu.

        \param toolWindow the tool window to add
    */
    virtual void addToolWindow(int index, ToolWindowBase* toolWindow);

    /** Create a new main window and show it. Derived classes must define.

        \param objectName the type of main window to create.

        \return new MainWindowBase object
    */
    virtual MainWindowBase* makeNewMainWindow(const QString& objectName) = 0;

    MainWindowBase* makeAndInitializeNewMainWindow(const QString& type);

private:

    std::string getInstallationRoot(const char* argv0);

    void setActiveConfiguration(const std::string& root);

    void setDocumentDirectory(const std::string& root);

    /** Manually set the active main window. MainWindowBase objects use this to notify the application when it
        becomes the active window.

        \param window the window that is now active
    */
    void setActiveWindow(QWidget* window);

    void setActiveMainWindow(MainWindowBase* window);

    /** Update the QAction associated with a given managed window to reflect the window's new title.

        \param window the managed window whose title has changed
    */
    void windowTitleChanged(QWidget* window);

    void makeLoggingMenuActions();

    void makeHelpMenuActions();

    bool event(QEvent* event);

    static void UpdateRadarConfig(const char* argv0);

    QString name_;
    QDir docDir_;
    WindowManager* windowManager_;
    ManualWindow* manualWindow_;
    LoggerWindow* loggerWindow_;
    QString lastConfigurationFile_;
    QString lastStyleSheetFile_;
    Logger::ConfiguratorFile* config_;
    QList<QAction*> toolsMenuActions_;
    QList<QAction*> loggingMenuActions_;
    QList<QAction*> helpMenuActions_;
    QString distanceUnits_;
    QString distanceUnitsSuffix_;
    QPointF phantomCursor_;
    AngularFormatType angularFormatType_;
    bool quitting_;
    DisableAppNap* disableAppNap_;

    static AppBase* singleton_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
