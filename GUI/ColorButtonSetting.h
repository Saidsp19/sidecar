#ifndef SIDECAR_GUI_COLORBUTTONSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_COLORBUTTONSETTING_H

#include "GUI/QColorSetting.h"

namespace SideCar {
namespace GUI {

class ColorButtonWidget;

/** Derivation of QColorSetting that works with a QPushButton widget to show the current color setting, and to
    provide a way to edit the color value when the button is pressed.
*/
class ColorButtonSetting : public QColorSetting
{
    Q_OBJECT
    using Super = QColorSetting;
public:

    /** Constructor with widget to manage.

        \param mgr PresetManager object that records this setting

        \param widget the QPushButton widget to use

        \param global if true, this is a global setting, not associated with a
        specific Preset.
    */
    ColorButtonSetting(PresetManager* mgr, ColorButtonWidget* widget,
                       bool global = false);
};

} // end namespace GUI
} // end namespace SideCar

#endif
