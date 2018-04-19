#ifndef SIDECAR_GUI_HEALTHANDSTATUS_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_HEALTHANDSTATUS_MAINWINDOW_H

#include "QtWidgets/QComboBox"

#include "GUI/MainWindowBase.h"

#include "App.h"
#include "ui_MainWindow.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

namespace HealthAndStatus {

class ChannelConnectionModel;
class ChannelPlotWidget;

/** Main window for the HealthAndStatus application.
 */
class MainWindow : public MainWindowBase, private Ui::MainWindow {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    static Logger::Log& Log();

    /** Constructor.

        \param history application history buffer for all Video messages.

        \param basis DisplayView to duplicate
    */
    MainWindow();

    /** Obtain the App object singleton.

        \return App object
    */
    App* getApp() const { return App::GetApp(); }

    /** Save window settings in a QSettings object. Override of MainWindowBase.

        \param settings container to hold the window settings
    */
    void saveToSettings(QSettings& settings);

    /** Restore a window configuration using settings from a QSettings object. Override of MainWindowBase.

        \param settings container with the window settings to use
    */
    void restoreFromSettings(QSettings& settings);

private slots:

    void on_actionAdd__triggered();

    void on_actionRemove__triggered();

    void on_actionClear__triggered();

    void on_actionMoveUp__triggered();

    void on_actionMoveDown__triggered();

    void currentPlotWidgetChanged(const QItemSelection& selected, const QItemSelection& deselected);

    void setUnconnected(const QStringList& names);

    void setAvailableNames(const QStringList& names);

    void updateButtons();

    void on_plots__clearDrops();

    void on_plots__clearAll();

private:
    void addPlot(const QString& name);

    void closeEvent(QCloseEvent* event);

    void setCurrentPlotWidget(int row);

    QModelIndexList getSelectedPlotWidgets() const;

    ChannelConnectionModel* model_;
    QComboBox* unconnected_;
};

} // end namespace HealthAndStatus
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
