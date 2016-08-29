#ifndef SIDECAR_GUI_CONFIGEDITOR_SETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_SETTINGS_H

#include "GUI/PresetManager.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class SettingsWindow;

/** Settings that persist between executions of the ConfigEditor application.
 */
class Settings : public PresetManager
{
    Q_OBJECT
    using Super = PresetManager;
public:

    Settings(SettingsWindow& gui);

private:
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
