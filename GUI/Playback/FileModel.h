#ifndef SIDECAR_GUI_PLAYBACK_FILEMODEL_H // -*- C++ -*-
#define SIDECAR_GUI_PLAYBACK_FILEMODEL_H

#include "QtCore/QAbstractTableModel"
#include "QtCore/QDir"
#include "QtCore/QList"
#include "QtCore/QStringList"

#include "Time/TimeStamp.h"

class QTimer;

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace Playback {

class Emitter;
class LoaderThread;
class MainWindow;

/** A model of Emitter objects. Provides display data for the MainWindow files_ attribute, a QTableView. Each
    entry in the model is an Emitter object. The model provides five columns of information to its QTableView
    clients:

    - \p kEmitting a checkbox that indicates whether the channel will emit data during playback
    - \p kName the name of the recording file used for playback (without the extension '.pri')
    - \p kStartTime the time of the first record in the recording file
    - \p kEndTime the time of the last record in the recording file
    - \p kDuration the about of time between kStart and kEnd

    The \p kStartTime and \p kEndTime fields contain times formatted as HH:MM:SS, with midnight of the day of the
    recording begin 00:00:00.

    The model manages playback start and stop for all of the held Emitter objects, and it maintains a playback clock
    which sends out wall time updates via the currentTime() signal.
*/
class FileModel : public QAbstractTableModel
{
    Q_OBJECT
    using Super = QAbstractTableModel;
public:

    /** Indices of the data columns provided by the model.
     */
    enum Columns {
	kEmitting = 0,
	kName,
	kStartTime,
	kEndTime,
	kDuration,
	kSubscriberCount,
	kNumColumns
    };

    /** Log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.

        \param parent owner of the object
    */
    FileModel(MainWindow* parent);

    /** Destructor.
     */
    ~FileModel();

    Emitter* getEmitter(int row) const { return emitters_[row]; }

    /** Create new Emitter objects for all of the recording files found in a the \p path directory.

        \param path location of the recording directory to process

	\return true if at least one Emitter object is valid
    */
    void beginLoad(const QString& path);

    /** Implementation of QAbstractItemModel method. Obtain the number of columns in the model data.

        \param parent model index of containing parent model (not used)

        \return number of columns (kNumColumns)
    */
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    /** Implementation of QAbstractItemModel method. Obtain the number of rows in the model data.

        \param parent model index of containing parent model (not used)

        \return number of rows
    */
    int rowCount( const QModelIndex& parent = QModelIndex()) const;

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
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    /** Implementation of QAbstractItemModel method. Change the data in the model at a particular location and
        display purpose.

        \param index location of the datum to set

        \param value new value to use

        \param role how the data will be used

        \return true if the value was successfully set, false otherwise.
    */
    bool setData(const QModelIndex& index, const QVariant& value,
                 int role = Qt::EditRole);

    /** Implementation of QAbstractItemModel method. Obtain the display flags for a particular location. All
        elements have Qt::ItemIsEnabled set, and the kVisible column has Qt::ItemIsUserCheckable set.

        \param index location of the flags to get

        \return display flags
    */
    Qt::ItemFlags flags(const QModelIndex& index) const;

    /** Obtain the earliest start time for all Emitter objects.

	\return earliest Time::TimeStamp value
    */
    const Time::TimeStamp& getStartTime() const { return startTime_; }

    /** Obtain the latest end time for all Emitter objects.

	\return Time::TimeStamp value
    */
    const Time::TimeStamp& getEndTime() const { return endTime_; }

signals:

    void loadComplete();

private slots:

    void updateLoadPercentage(int row);

    void updateSubscriberCount(int row);

    void suffixChanged(const QString& suffix);

    void loaderFinished();

    void finishedLoading();

private:
    MainWindow* parent_;
    using EmitterList = QList<Emitter*>;
    EmitterList emitters_;
    Time::TimeStamp startTime_;
    Time::TimeStamp endTime_;
    QString suffix_;
    using LoaderThreadList = QList<LoaderThread*>;
    LoaderThreadList loaders_;
    QStringList failures_;
    struct LoaderProgress;
    LoaderProgress* loaderMeter_;
    QDir dir_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
