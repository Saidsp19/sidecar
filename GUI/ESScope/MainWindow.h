#ifndef SIDECAR_GUI_ESSCOPE_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_MAINWINDOW_H

#include "QtCore/QBasicTimer"

#include "GUI/MainWindowBase.h"
#include "GUI/MessageList.h"

#include "App.h"
#include "ui_MainWindow.h"

class QGridLayout;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace ESScope {

class AlphaBetaView;
class AlphaBetaViewSettings;
class AlphaRangeView;
class AlphaRangeViewSettings;
class Configuration;
class CursorWidget;
class PastImage;
class PPIWidget;
class ScaleWidget;

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

private slots:

    void on_actionPresetRevert__triggered();
    void on_actionPresetSave__triggered();

    void on_actionAlphaBetaViewFull__triggered();
    void on_actionAlphaBetaViewPrevious__triggered();
    void on_actionAlphaBetaViewSwap__triggered();

    void on_actionAlphaRangeViewFull__triggered();
    void on_actionAlphaRangeViewPrevious__triggered();
    void on_actionAlphaRangeViewSwap__triggered();

    void updatePresetActions();

    void updateAlphaBetaViewActions();
    void updateAlphaRangeViewActions();

    void bugged();

    void on_actionViewClearAll__triggered();
    void on_actionViewClearVideo__triggered();
    void on_actionViewClearExtractions__triggered();
    void on_actionViewClearRangeTruths__triggered();
    void on_actionViewClearBugPlots__triggered();

private:
    void timerEvent(QTimerEvent* event);
    void showEvent(QShowEvent* event);
    void closeEvent(QCloseEvent* event);

    AlphaBetaView* alphaBetaView_;
    AlphaBetaViewSettings* alphaBetaViewSettings_;
    AlphaRangeView* alphaRangeView_;
    AlphaRangeViewSettings* alphaRangeViewSettings_;
    QBasicTimer updateTimer_;
    Configuration* configuration_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
