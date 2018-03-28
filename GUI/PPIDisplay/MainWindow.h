#ifndef SIDECAR_GUI_PPIDISPLAY_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_MAINWINDOW_H

#include "GUI/MainWindowBase.h"

#include "App.h"
#include "ui_MainWindow.h"

namespace SideCar {
namespace GUI {

class UDPMessageWriter;

namespace PPIDisplay {

/** Main display window for the PPIDisplay application. Hosts a PPIWidget display widget and various GUI
    controls that affect the display. The actual GUI controls are defined in the PPIDisplayWindow.ui file.
*/
class MainWindow : public MainWindowBase, private Ui::MainWindow {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    static Logger::Log& Log();

    /** Constructor. Creates all of the widgets connects signals and slots.

        \param parent
    */
    MainWindow();

    App* getApp() const { return App::GetApp(); }

    PPIWidget* getDisplay() const { return display_; }

private slots:
    void on_actionViewZoomIn__triggered();
    void on_actionViewZoomOut__triggered();
    void on_actionViewPanLeft__triggered();
    void on_actionViewPanRight__triggered();
    void on_actionViewPanUp__triggered();
    void on_actionViewPanDown__triggered();
    void on_actionViewCenterAtCursor__triggered();
    void on_actionViewReset__triggered();
    void on_actionViewClearAll__triggered();
    void on_actionViewClearVideo__triggered();
    void on_actionViewClearBinaryVideo__triggered();
    void on_actionViewClearExtractions__triggered();
    void on_actionViewClearRangeTruths__triggered();
    void on_actionViewClearBugPlots__triggered();
    void on_actionPresetRevert__triggered();
    void on_actionPresetSave__triggered();
    void updatePresetActions();
    void monitorPresets(int index, bool isDirty);

private:
    void saveToSettings(QSettings& settings);

    void restoreFromSettings(QSettings& settings);

    void showEvent(QShowEvent* event);

    void closeEvent(QCloseEvent* event);

    UDPMessageWriter* writer_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
