#ifndef SIDECAR_GUI_BSCOPE_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_MAINWINDOW_H

#include "GUI/MainWindowBase.h"

#include "App.h"
#include "ui_MainWindow.h"

class QGridLayout;

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace BScope {

class CursorWidget;
class HistorySettings;
class PastImage;
class PPIWidget;
class ScaleWidget;
class ViewSettings;

/** Main display window for the BScope application. Hosts a PPIWidget display widget and various GUI controls
    that affect the display. The actual GUI controls are defined in the BScopeWindow.ui file.
*/
class MainWindow : public MainWindowBase, private Ui::MainWindow
{
    Q_OBJECT
    using Super = MainWindowBase;
public:
    
    static Logger::Log& Log();

    /** Constructor. Creates all of the widgets connects signals and slots.

        \param parent 
    */
    MainWindow();

    void initialize();

    App* getApp() const { return App::GetApp(); }

    PPIWidget* getDisplay() const { return display_; }

    QMenu* getViewMenu() const { return menuView_; }

private slots:

    void setPhantomCursor(const QPointF& pos);

    void displayTransformChanged();

    void rangeRingsImagingChanged();

    void on_actionViewClearAll__triggered();
    void on_actionViewClearVideo__triggered();
    void on_actionViewClearBinaryVideo__triggered();
    void on_actionViewClearExtractions__triggered();
    void on_actionViewClearRangeTruths__triggered();
    void on_actionViewClearBugPlots__triggered();

    void on_actionShowPlayerWindow__triggered();

    void on_actionShowFramesWindow__triggered();

    void on_actionPresetRevert__triggered();

    void on_actionPresetSave__triggered();

    void updatePresetActions();

    void monitorPresets(int index, bool isDirty);

private:

    void showEvent(QShowEvent* event);

    void closeEvent(QCloseEvent* event);

    void saveToSettings(QSettings& settings);

    void restoreFromSettings(QSettings& settings);

    QMenu* contextMenu_;
    PPIWidget* display_;
    ScaleWidget* rangeScale_;
    ScaleWidget* azimuthScale_;
    ViewSettings* viewSettings_;
    HistorySettings* historySettings_;
    double rangeConversion_;
    double azimuthConversion_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
