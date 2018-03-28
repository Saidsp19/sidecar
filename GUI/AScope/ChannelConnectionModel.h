#ifndef SIDECAR_GUI_ASCOPE_CHANNELCONNECTIONMODEL_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_CHANNELCONNECTIONMODEL_H

#include "QtCore/QAbstractTableModel"
#include "QtCore/QHash"
#include "QtCore/QSet"

#include "GUI/ServiceBrowser.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace AScope {

class ChannelConnection;
class ChannelConnectionWindow;
class History;
class PeakBarSettings;
class VideoChannel;
class Visualizer;

/** A model of channel connection objects for a specific Visualizer (only one Visualizer is the active one at
    any time). The model supplies the data for display by a ChannelConnectionView view, and it implements most
    of the controller aspects of the MVC paradigm.
*/
class ChannelConnectionModel : public QAbstractTableModel {
    Q_OBJECT
    using Super = QAbstractTableModel;

public:
    /** Mapping of VideoChannel objects to their name.
     */
    using VideoChannelHash = QHash<QString, VideoChannel*>;

    /** Indices of the data columns provided by the model.
     */
    enum Columns {
        kName = 0,
        kColor,
        kSampleMin,
        kSampleMax,
        kVoltageMin,
        kVoltageMax,
        kVisible,
        kFrozen,
        kShowPeakBars,
        kDisplayCount,
        kNumColumns
    };

    /** Obtain the log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.

        \param parent owner of the object
    */
    ChannelConnectionModel(ChannelConnectionWindow* parent);

    /** Destructor. Shuts down held VideoChannel objects.
     */
    ~ChannelConnectionModel();

    /** Obtain the VideoChannel object with a given name

        \param channelName name to look for

        \return found object or NULL
    */
    VideoChannel* getVideoChannel(const QString& channelName);

    /** Obtain the ChannelConnection object associated with \a row.

        \param row the row to fetch

        \return ChannelConnection object or NULL if \a row is invalid
    */
    ChannelConnection* getChannelConnection(int row) const;

    /** Set the active Visualizer object. May be NULL.

        \param visualizer Visualizer object to use
    */
    void setVisualizer(Visualizer* visualizer);

    /** Create a new ChannelConnection object to link the active Plotter object with the VideoChannel object
        having a given name. If there is no VideoChannel object found with the given name, create one.

        \param name data channel name

        \return true if successful, false otherwise
    */
    bool makeConnection(const QString& name);

    /** Remove an existing connections.

        \param index index of the ChannelConnection object to remove

        \return ServiceEntry of the connection that was removed if still
        available, or NULL if service is no longer present
    */
    ServiceEntry* removeConnection(int index);

    /** Implementation of QAbstractItemModel method. Obtain the number of columns in the model data.

        \param parent model index of containing parent model (not used)

        \return number of columns (kNumColumns)
    */
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    /** Implementation of QAbstractItemModel method. Obtain the number of rows in the model data.

        \param parent model index of containing parent model (not used)

        \return number of rows
    */
    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    /** Implementation of QAbstractItemModel method. Obtain a piece of data from the model for a particular
        location (row,column) and purpose.

        \param index location of the datum to get

        \param role how the data will be used

        \return value to use for the given purpose
    */
    QVariant data(const QModelIndex& pos, int role = Qt::DisplayRole) const;

    /** Implementation of QAbstractItemModel method. Obtain a piece of data from the model to display in the
        horizontal or vertical header. The horizontal header contains column names, while the vertical header
        contains row numbers (one-based).

        \param section which row or column to fetch

        \param orientation which header, horizontal or vertical, to fetch for

        \param role how the data will be used

        \return value to use for the given purpose
    */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    /** Implementation of QAbstractItemModel method. Change the data in the model at a particular location and
        display purpose.

        \param index location of the datum to set

        \param value new value to use

        \param role how the data will be used

        \return true if the value was successfully set, false otherwise.
    */
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    /** Implementation of QAbstractItemModel method. Obtain the display flags for a particular location. All
        elements have Qt::ItemIsEnabled set, and the kVisible column has Qt::ItemIsUserCheckable set.

        \param index location of the flags to get

        \return display flags
    */
    Qt::ItemFlags flags(const QModelIndex& index) const;

    /** Move a row up in the list of channel connections. Rows higher are drawn before those below them.

        \param row index of the ChannelConnection object to move
    */
    void moveUp(int row);

    /** Move a row down in the list of channel connections. Rows higher are drawn before those below them.

        \param row index of the ChannelConnection object to move
    */
    void moveDown(int row);

    /** Write current VideoChannel settings the to application's settings file.
     */
    void saveVideoChannelSettings();

private slots:

    /** Notification from a VideoChannel object that its connection status has changed.
     */
    void channelConnectionChanged();

    /** Notification from a ServiceBrowser object of a new set of active ServiceEntry objects.

        \param services
    */
    void setAvailableServices(const ServiceEntryHash& services);

    void peakBarSettingsEnabledChanged(bool state);

private:
    /** Find the VideoChannel object with a given channel \a name.

        \param name the name to look for

        \return VideoChannel object or NULL if none found
    */
    VideoChannel* findVideoChannel(const QString& name) const;

    /** Locate the row in the model that refers to a specific VideoChannel connection.

        \param channel the VideoChannel to look for

        \return row index or -1 if not found
    */
    int findChannelConnectionRow(VideoChannel* channel) const;

    /** Locate the row in the model that is connected to a channel with a given name.

        \param name channel name to look for

        \return row index or -1 if not found
    */
    int findChannelConnectionRow(const QString& name) const;

    /** Obtain the parent of this object.

        \return ChannelConnectionWindow object
    */
    ChannelConnectionWindow* getParent() const;

    /** Record and link a new VideoChannel object

        \param channel VideoChannel object to add
    */
    void addVideoChannel(VideoChannel* channel);

    PeakBarSettings& peakBarSettings_;
    History& history_;
    Visualizer* visualizer_;
    ServiceBrowser* browser_;
    VideoChannelHash channels_;
    QSet<QString> available_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
