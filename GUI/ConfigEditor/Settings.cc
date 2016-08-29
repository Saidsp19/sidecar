#include "App.h"
#include "Settings.h"
#include "SettingsWindow.h"

using namespace SideCar::GUI::ConfigEditor;

Settings::Settings(SettingsWindow& win)
    : Super("Settings")
{
    restoreAllPresets();
}
