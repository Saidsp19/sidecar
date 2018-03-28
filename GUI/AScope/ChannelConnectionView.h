#ifndef SIDECAR_GUI_ASCOPE_CHANNELCONNECTIONVIEW_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_CHANNELCONNECTIONVIEW_H

#include "QtGui/QTableView"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace AScope {

/** View class for channel connections:

    - QCheckBox indicating whether the channel data is visible
    - Text showing the name of the channel, italicized if the channel is not
    connected to a publisher.
    - Color swatch showing the display color of the channel. If the user
    double-clicks on the swatch, the view shows a QColorDialog box so that
    the user may change the color value.
*/
class ChannelConnectionView : public QTableView {
    Q_OBJECT
    using Super = QTableView;

public:
    static Logger::Log& Log();

    /** Constructor. Sets up the view to respond to user clicks. Sets up the header.

        \param parent
    */
    ChannelConnectionView(QWidget* parent = 0);

private slots:

    /** Handler invoked when the user double-clicks on a row. Pops up an appropriate editor for the column that
        was clicked on.

        \param index contains the row that was double-clicked on
    */
    void editEntry(const QModelIndex& index);

    /** Notification handler invoked when one or more rows have been inserted into the model. Resizes the
        columns so that the name column is as big as possible.

        \param parent parent index of the collection (ignored)

        \param start index of the first fow inserted

        \param end index of the last row inserted
    */
    void rowsInserted(const QModelIndex& parent, int start, int end);

    /** Notification handler invoked when one or more rows have been removed from the model. Resizes the columns
        so that the name column is as big as possible.

        \param parent parent index of the collection (ignored)

        \param start index of the first fow inserted

        \param end index of the last row inserted
    */
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);

    /** Set the visibility of all channels to true.
     */
    void makeAllVisible();

    /** Set the visibility of all channels to false.
     */
    void makeAllInvisible();

    /** Remove all existing channel connections.
     */
    void removeAll();

private:
    /** Adjust the columns such that the visible and color coluns take up a minimum amount of space, and the
        name column fills out the view.
    */
    void adjustColumnSizes();

    /** Override of QWidget event handler. Detect when the view changes size in order to adjust the column
        widths.

        \param event ignored
    */
    void resizeEvent(QResizeEvent* event);

    /** Override of QWidget event handler. Posts the context menu for the user.

        \param event event descriptor
    */
    void contextMenuEvent(QContextMenuEvent* event);

    QMenu* contextMenu_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
