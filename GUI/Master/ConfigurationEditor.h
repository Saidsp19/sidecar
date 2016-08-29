#ifndef SIDECAR_GUI_MASTER_CONFIGURATIONEDITOR_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_CONFIGURATIONEDITOR_H

#include "GUI/MainWindowBase.h"

namespace Ui { class ConfigurationEditor; }
namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace Master {

class ConfigurationInfo;

class ConfigurationEditor : public MainWindowBase 
{
    Q_OBJECT
    using Super = MainWindowBase;
public:

    /** Constructor.
     */
    ConfigurationEditor(ConfigurationInfo& info);

    void setCursorPosition(int line, int column);

private slots:

    void textChanged();
    void on_actionSave_triggered();
    void on_actionRevert_triggered();

private:

    void makeMenuBar();

    void closeEvent(QCloseEvent* event);

    bool save();

    Ui::ConfigurationEditor* gui_;
    ConfigurationInfo& info_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
