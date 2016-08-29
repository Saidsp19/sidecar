#ifndef SIDECAR_GUI_ZEROCONFMONITOR_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_ZEROCONFMONITOR_MAINWINDOW_H

#include "QtCore/QList"
#include "QtCore/QPointer"

#include "GUI/MainWindowBase.h"
#include "GUI/ServiceBrowser.h"
#include "IO/ZeroconfRegistry.h"

#include "App.h"

#include "ui_MainWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace ZeroconfMonitor {

/** Main window for the ZeroconfMonitor application.
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
};

} // end namespace ZeroconfMonitor
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
