#include "Settings.h"
#include "App.h"
#include "SettingsWindow.h"

using namespace SideCar::GUI::ConfigEditor;

Settings::Settings(SettingsWindow& win) : Super("Settings")
{
    restoreAllPresets();
}
