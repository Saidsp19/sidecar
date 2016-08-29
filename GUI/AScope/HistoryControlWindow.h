#ifndef SIDECAR_GUI_ASCOPE_HISTORYCONTROLWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_HISTORYCONTROLWINDOW_H

#include "GUI/ToolWindowBase.h"
#include "Messages/PRIMessage.h"

#include "ui_HistoryControlWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class MainWindowBase;

namespace AScope {

class History;
class HistoryFrame;
class HistoryPosition;

/** Tool window that allows the user to move back in time to a previous frame of data. Shows information about
    the previous frame as well as the size of the history buffer. It does not manipulate the application's
    History object, only reports information about it. It will change the active MainWindow's HistoryPosition
    when the user adjusts the history position slider.
*/
class HistoryControlWindow : public ToolWindowBase,
			     private Ui::HistoryControlWindow
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:

    static Logger::Log& Log();

    /** Constructor. Creates and initializes window widgets.

	\param shortcut key sequence to change window visibility

	\param history application History object to work with
    */
    HistoryControlWindow(int shortcut, const History* history);

public slots:
    
    /** Signal handler invoked when the active MainWindow changes.

        \param window new active window
    */
    void activeMainWindowChanged(MainWindowBase* window);

private slots:

    /** Action handler for the current push button.
     */
    void on_current__clicked();

    /** Action handler for the slider widget

        \param action 
    */
    void on_slider__actionTriggered(int action);

    /** Notification from the active HistoryPosition object that a new frame is visible.

        \param frame the new frame
    */
    void historyPositionViewChanged(const HistoryFrame* frame);

    /** Notification from the active HistoryPosition object when its position value changes.

        \param position new position value

        \param age the age of the frame the position refers to with respect to
        the current time
    */
    void historyPositionPositionChanged(int position, double age);

private:
    
    /** Change the position held by the active HistoryPosition object. Called in response to a slider event or
        the current push button.

        \param position new position value
    */
    void setPosition(int position);
    
    /** Setup the connection to the active HistoryPosition object.

        \param historyPosition active HistoryPosition object to use
    */
    void installHistoryPosition(HistoryPosition* historyPosition);

    const History* history_;
    HistoryPosition* historyPosition_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
