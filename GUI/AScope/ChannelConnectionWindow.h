#ifndef SIDECAR_GUI_ASCOPE_CHANNELCONNECTIONWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_CHANNELCONNECTIONWINDOW_H

#include "GUI/ToolWindowBase.h"

#include "ui_ChannelConnectionWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace AScope {

class ChannelConnection;
class ChannelConnectionModel;
class DisplayView;
class History;
class PeakBarSettings;
class VideoChannel;

/** Tool window that shows the list of configured channel connections for the active Visuazlier. Hosts buttons
    that allow the user to add/remove channel connections, and to change the ordering of the existing
    connections.
*/
class ChannelConnectionWindow : public ToolWindowBase,
				private Ui::ChannelConnectionWindow
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:

    /** Obtain the Log device to use for log messages from this class.

        \return Log reference
    */
    static Logger::Log& Log();

    /** Constructor. Creates and initializes the GUI objects in the window.

	\param shortcut key sequence to change window's visibility

        \param history application's data history buffer
    */
    ChannelConnectionWindow(int shortcut);

    /** Obtain the VideoChannel object with a given name.

        \param channelName the name to look for

	\return found VideoChannel object, or NULL
    */
    VideoChannel* getVideoChannel(const QString& channelName);

    /** Obtain the ChannelConnection represented by the current selection. Note that there may not be a
        selection, in which case this will return NULL.

        \return ChannelConnection object or NULL
    */
    ChannelConnection* getSelectedChannelConnection() const;

    /** Use the given list of service names as the contents of the QComboBox widget that shows the services that
        are unconnected.

        \param values service names
    */
    void setUnconnected(const QStringList& values);

    /** Obtain the list of unconnected channels

        \return QString list
    */
    QStringList getUnconnected() const;

public slots:
    
    /** Notification that the active DisplayView has changed. Notify the model object of the change by giving it
        the new DisplayView's Plotter.

        \param displayView the active DisplayView (may be NULL)
    */
    void activeDisplayViewChanged(DisplayView* displayView);

private slots:

    /** Action handler for the Add button. The data model creates a new ChannelConnection object for the service
	name shown in the QComboBox widget that contains the unconfigured services.
    */
    void on_add__clicked();

    /** Action handler for the Remove button. Removes one or more ChannelConnection objexts. Places the names of
	the removed services into the QComboBox widget for unconfigured services if a service exists under the
	given name.
    */
    void on_remove__clicked();

    /** Action handler for the up arrow button. Move the selected above another one.
     */
    void on_moveUp__clicked();

    /** Action handler for the down arrow button. Move the selected below another one.
     */
    void on_moveDown__clicked();

    /** Notification handler invoked when the underlying model row count changes or the user's selection
        changes.
    */
    void updateWidgets();

private:

    void closeEvent(QCloseEvent*event);

    void keyPressEvent(QKeyEvent*event);

    ChannelConnectionModel* model_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
