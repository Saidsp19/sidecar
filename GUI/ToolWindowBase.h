#ifndef SIDECAR_GUI_TOOLWINDOWBASE_H // -*- C++ -*-
#define SIDECAR_GUI_TOOLWINDOWBASE_H

#include "QtWidgets/QDialog"

namespace Logger {
class ConfiguratorFile;
class Log;
} // namespace Logger

namespace SideCar {
namespace GUI {

class AppBase;
class MainWindowBase;

/** Common base class for all SideCar GUI tool windows. A tool window is a QDialog window that is not modal. The
    application saves and restores tool window position, size, and visibility state information.
*/
class ToolWindowBase : public QDialog {
    Q_OBJECT
    using Super = QDialog;

public:
    /** Log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.

        \param name the name of the tool window to use when loading and saving
        settings

        \param actionTitle text for the QAction object used to control window
        visibility

        \param shortcut optional key sequence used to toggle window visibility
        reflects the state of the tool window
    */
    ToolWindowBase(const QString& name, const QString& actionTitle, int shortcut = 0);

    /** Obtain the QAction object that toggles the tool window's visibility.

        \return QAction object
    */
    QAction* getShowHideAction() const { return showHideAction_; }

    /** Configure the window so that the user may not resize it.
     */
    void setFixedSize();

    /** Determine if the application should show the window at startup.

        \return true if so
    */
    bool getInitialVisibility() const;

    /** Convenience method that obtains the AppBase singleton.

        \return AppBase object
    */
    static AppBase* getApp();

public slots:

    virtual void activeMainWindowChanged(MainWindowBase* window);

    virtual void applyInitialVisibility();

    virtual void toggleVisibility();

    virtual void showAndRaise();

protected:
    /** Event handler for show events. Use any saved settings for the window's size and position values. Updates
        any managed QAction object.

        \param event event descriptor
    */
    void showEvent(QShowEvent* event);

    /** Event handler for close events. Update any managed QAction object.

        \param event event descriptor
    */
    void closeEvent(QCloseEvent* event);

    /** Event handler for hide events. Update any managed QAction object.

        \param event event descriptor
    */
    void hideEvent(QHideEvent* event);

private:
    void updateShowHideMenuAction(bool state);

    QAction* showHideAction_;
    bool restoreFromSettings_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
