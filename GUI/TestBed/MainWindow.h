#ifndef SIDECAR_GUI_TESTBED_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_TESTBED_MAINWINDOW_H

#include "GUI/MainWindowBase.h"

#include "App.h"

#include "ui_TestBed.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

namespace TestBed {

/** Main window for the TestBed application.
 */
class MainWindow : public MainWindowBase, private Ui::TestBed
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

public slots:

    void on_actionA__clicked();
    void on_actionB__clicked();
    void on_actionC__clicked();
    void on_actionD__clicked();

    void on_modeA__clicked();
    void on_modeB__clicked();
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
