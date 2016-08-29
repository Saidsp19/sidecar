#ifndef SIDECAR_GUI_SPECTRUM_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_MAINWINDOW_H

#include "GUI/MainWindowBase.h"

#include "App.h"

#include "ui_MainWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

namespace Spectrum {

class CursorWidget;
class FreqScaleWidget;
class PowerScaleWidget;
class SpectrumWidget;

/** Main window for the Spectrum application.
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
    MainWindow();

    /** Obtain the App object singleton.

        \return App object
    */
    App* getApp() const { return App::GetApp(); }

    SpectrumWidget* getDisplay() const { return display_; }

    void saveToSettings(QSettings& settings);

    void restoreFromSettings(QSettings& settings);
						   
private slots:

    void showCursorPosition(const QPointF& pos);

    void updateScales();

    void transformChanged();

    void makeBackground();

    void on_actionViewPause__triggered(bool state);

    /** Action handler that restores a prior visual transform.
     */
    void on_actionViewPrevious__triggered();

    void on_actionViewSwap__triggered();

    /** Action handler that restores the full visual transform (the one described by the configuration)
     */
    void on_actionViewFull__triggered();

    void on_actionShowSpectrograph__triggered();

    void channelChanged(const QString& name);

private:

    void closeEvent(QCloseEvent* event);

    SpectrumWidget* display_;
    PowerScaleWidget* powerScale_;
    FreqScaleWidget* freqScale_;
    CursorWidget* cursorWidget_;
    bool showGrid_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
