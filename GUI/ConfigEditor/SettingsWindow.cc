#include "SettingsWindow.h"
#include "App.h"

#include "ui_SettingsWindow.h"

using namespace SideCar::GUI::ConfigEditor;

SettingsWindow::SettingsWindow(int shortcut) :
    ToolWindowBase("SettingsWindow", "Settings", shortcut), gui_(new Ui::SettingsWindow)
{
    gui_->setupUi(this);
    setFixedSize();
}
