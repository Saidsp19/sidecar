#ifndef SIDECAR_GUI_ASCOPE_HISTORYCONTROLWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_HISTORYCONTROLWIDGET_H

#include "Messages/PRIMessage.h"

#include "ui_HistoryControlWidget.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class ToolBar;

namespace AScope {

class DisplayView;
class History;
class HistoryPosition;

/** Tool window that allows the user to move back in time to a previous frame of data. Shows information about
    the previous frame as well as the size of the history buffer. It does not manipulate the application's
    History object, only reports information about it. It will change the active MainWindow's HistoryPosition
    when the user adjusts the history position slider.
*/
class HistoryControlWidget : public QWidget, private Ui::HistoryControlWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    static Logger::Log& Log();

    /** Constructor. Creates and initializes window widgets.

        \param shortcut key sequence to change window visibility

        \param history application History object to work with
    */
    HistoryControlWidget(ToolBar* parent);

    void manage(HistoryPosition* historyPosition);

signals:

    void positionChange(int);

public slots:

    void activeDisplayViewChanged(DisplayView* displayView);

private slots:

    void on_viewPast__toggled(bool checked);

    void on_synch__toggled(bool checked);

    void on_pastSlider__valueChanged(int value);

    void pastFrozen();

    void pastThawed();

    void viewingPastChanged(bool state);

    void historyEnabledChanged(bool state);

private:
    History& history_;

    HistoryPosition* activeHistoryPosition_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
