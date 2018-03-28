#ifndef SIDECAR_GUI_CONFIGEDITOR_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_MAINWINDOW_H

#include "GUI/MainWindowBase.h"

#include "App.h"

namespace Ui {
class MainWindow;
}

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class TreeModel;

class MainWindow : public MainWindowBase {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    /** Constructor.
     */
    MainWindow();

    /** Obtain the App object singleton.

        \return App object
    */
    App* getApp() const { return App::GetApp(); }

private:
    Ui::MainWindow* gui_;
    TreeModel* model_;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
