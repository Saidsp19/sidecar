#ifndef SIDECAR_GUI_PLAYBACK_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_PLAYBACK_MAINWINDOW_H

#include "GUI/MainWindowBase.h"
#include "Time/TimeStamp.h"

#include "ui_MainWindow.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Playback {

class BrowserWindow;
class Clock;
class FileModel;
class NotesWindow;

/** Main window for the Playback application. Contains all of the GUI elements via inheritance from
    Ui::MainWindow
*/
class MainWindow : public MainWindowBase, private Ui::MainWindow {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    /** Log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.
     */
    MainWindow();

    /** Obtain the multicast address configured by the user.

        \return QString value
    */
    QString getAddress() const { return address_->text(); }

    /** Obtain the channel publisher suffix configured by the user.

        \return QString value
    */
    QString getSuffix() const { return suffix_->text(); }

    /** Obtain the current wall clock rate setting.

        \return wall clock rate value
    */
    double getWallClockRate() const;

    /**

        \return
    */
    Clock* getClock() const { return clock_; }

signals:

    /** Notification sent out when the user changes the multicast IP address

        \param address new value
    */
    void addressChanged(const QString& address);

    /** Notification sent out when the user changes the channel name suffix

        \param suffix new value
    */
    void suffixChanged(const QString& suffix);

private slots:

    /** Action handler for the Load push button. Presents the user with an open dialog box with which to select
        a recording directory to load. Unless cancelled, the routine will invoke the FileModel::load() method
        with the chosen directory path.
    */
    void on_load__clicked();

    void on_actionLoad__triggered();

    void on_actionStart__triggered();

    void on_actionRewind__triggered();

    void openRecentDir();

    /** Action handler for the Notes push button. Opens read-only text window that shows the contents of the
        'notes.txt' file from the recording directory.
    */
    void on_notes__clicked();

    /** Action handler for the multicast IP address data entry field. Called when the user leaves the field, or
        pressed ENTER. Saves the new value to the application setttings file, and emits addressChanged() with
        the new value.
    */
    void on_address__editingFinished();

    /** Action handler for the suffix data entry field. Called when the user leaves the field, or pressed ENTER.
        Saves the new value to the application setttings file, and emits suffixChanged() with the new value.
    */
    void on_suffix__editingFinished();

    /** Action handler for the Start/Stop push button. Starts or stops playback.
     */
    void on_startStop__clicked();

    /** Action handler for the Rewind push button
     */
    void on_rewind__clicked();

    /** Action handler for the jumpTo entry field. Called when the user enters a new value or selects a previous
        value from the pull-down menu.

        \param index the index of the current entry in the pull-down menu
    */
    void on_jumpTo__activated(int index);

    void on_bookmarks__activated(int index);

    void on_bookmarkAdd__clicked();

    void on_bookmarkDelete__clicked();

    void on_regionLoop__currentIndexChanged(int index);

    void on_regionStart__activated(int index);

    void on_regionEnd__activated(int index);

    /** Action handler for the rate slider. Called when the current value changes.

        \param index the index of the current entry
    */
    void on_rateMultiplier__currentIndexChanged(int index);

    void on_recordingDir__activated(int index);

    /** Notification from the Clock that it has started.
     */
    void clockStarted();

    /** Notification from the Clock that it has stopped.
     */
    void clockStopped();

    /** Notifcation from the FileModel of the new playback clock time.

        \param currentTime
    */
    void clockTick(const Time::TimeStamp& playbackClock, const Time::TimeStamp& elapsed);

    void playbackClockStartChanged(const Time::TimeStamp& playbackClock);

    void updateColumns(const QModelIndex& topLeft, const QModelIndex& bottomRight);

    /** Attempt to use \a path as a recording directory, loading in the recording files there.

        \param path directory to load
    */
    void load(const QString& path);

    void loaded();

private:
    void writeBookmarks();

    Time::TimeStamp createTimeStamp(QString spec) const;

    void updateRecentFileActions();

    bool eventFilter(QObject* object, QEvent* event);

    void closeEvent(QCloseEvent* event);

    void adjustColumnSizes();

    Clock* clock_;
    FileModel* model_;
    NotesWindow* notesWindow_;
    QString pendingPath_;
    QString activePath_;
    int wallClockRatePower_;
    Time::TimeStamp regionStartTime_;
    Time::TimeStamp regionEndTime_;
    Time::TimeStamp zero_;
    enum { kMaxRecentFiles = 10 };
    QAction* recentFiles_[kMaxRecentFiles];
};

} // namespace Playback
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
