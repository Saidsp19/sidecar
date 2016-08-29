#ifndef SIDECAR_GUI_ASCOPE_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_MAINWINDOW_H

#include "QtCore/QList"
#include "QtCore/QPointer"

#include "GUI/MainWindowBase.h"

#include "App.h"
#include "DisplayView.h"

#include "ui_MainWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class Splittable;

namespace AScope {

class AzimuthLatch;
class ChannelConnectionWindow;
class ConfigurationWindow;
class CursorWidget;
class HistoryControlWidget;
class RadarInfoWidget;
class Visualizer;

/** Main window for the AScope application. A main window hosts one or more DisplayView objects, and each
    DisplayView object has a Visualizer widget which does the plotting of the incoming data.
*/
class MainWindow : public MainWindowBase, private Ui::MainWindow
{
    Q_OBJECT
    using Super = MainWindowBase;
public:

    static Logger::Log& Log();

    /** Constructor.

        \param history application history buffer for all Video messages.

	\param basis DisplayView to duplicate
    */
    MainWindow(History& history, int windowIndex, DisplayView* basis = 0);

    /** Obtain the active DisplayView for the window. When a window is raised to become the active window, it
        installs its active DisplayView to be the application's active DisplayView object.NOTE: this value
        should never be NULL.

        \return DisplayView object
    */
    DisplayView* getActiveDisplayView() const { return activeDisplayView_; }

    /** Determine if the active display view is contained in a split view.

        \return true if so
    */
    bool isDisplayViewSplit() const;

    /** Obtain the App object singleton.

        \return App object
    */
    App* getApp() const { return App::GetApp(); }

    /** Write out settings of the window and its contents to a given QSettings object.

        \param settings container to store the setting values
    */
    void saveToSettings(QSettings& settings);

    /** Restore settings for the window and its contents from a QSettings object.

        \param settings container to use for setting values
    */
    void restoreFromSettings(QSettings& settings);

    void windowMenuAboutToShow(QList<QAction*>& actions);
							   
private slots:

    /** Notification handler invoked when a display view goes away.

        \param object the DisplayView object that was destroyed
    */
    void displayViewDeleted(QObject*);

    /** Split a DisplayView with a new Qsplitter set to the middle of the DisplayView that currently has focus.
     */
    void splitDisplayViewHorizontally();

    /** Split a DisplayView with a new Qsplitter set to the middle of the DisplayView that currently has focus.
     */
    void splitDisplayViewVertically();

    /** Remove a QSplitter, keeping the DisplayView object that currently has focus. Destroyes the other
	DisplayView object.
    */
    void unsplitDisplayView();

    void on_actionToggleHorizontalScale__triggered();

    void on_actionToggleVerticalScale__triggered();

    /** Action handler that restores a prior visual transform.
     */
    void on_actionViewToggleGrid__triggered();

    /** Action handler that restores a prior visual transform.
     */
    void on_actionViewTogglePeaks__triggered();

    void on_actionViewPause__triggered();

    /** Action handler that restores a prior visual transform.
     */
    void on_actionViewPrevious__triggered();

    void on_actionViewSwap__triggered();

    /** Action handler that restores the full visual transform (the one described by the configuration)
     */
    void on_actionViewFull__triggered();

    /** Action handler that splits the active view horizontally.
     */
    void on_actionViewSplitHorizontally__triggered();

    /** Action handler that splits the active view vertically.
     */
    void on_actionViewSplitVertically__triggered();

    /** Action handler that unsplits a view.
     */
    void on_actionViewUnsplit__triggered();

    /** Notification that

        \param displayView 
    */
    void activeDisplayViewChanged(DisplayView* displayView);

    /** Notification that the active Visualizer's transform changed. Update the menu actions associated with the
	ViewStack.
    */
    void updateViewActions();

    void peakBarsEnabledChanged(bool state);

private:

    QWidget* makeDisplayView(QWidget* parent, DisplayView* basis);

    Splittable* splitDisplayView(Qt::Orientation how);

    Splittable* getActiveSplittable() const;

    void saveSplittable(QSettings& settings, Splittable* splittable);

    void restoreSplittable(QSettings& settings, Splittable* splittable);

    void actionToggleHorizontalScaleUpdate(bool state);

    void actionToggleVerticalScaleUpdate(bool state);

    AzimuthLatch* azimuthLatch_;
    CursorWidget* cursorWidget_;
    RadarInfoWidget* radarInfoWidget_;
    HistoryControlWidget* historyControlWidget_;
    QPointer<DisplayView> activeDisplayView_;
    QList<DisplayView*> displayViews_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
