#ifndef SIDECAR_GUI_QSPINBOXSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_QSPINBOXSETTING_H

#include "GUI/IntSetting.h"

class QSpinBox;

namespace SideCar {
namespace GUI {

/** Derivation of IntSetting that works with a QSpinBox widget to display and change setting values. Uses the
    QSpinBox::editingFinished() signal to detect changes in the widget.
*/
class QSpinBoxSetting : public IntSetting
{
    Q_OBJECT
public:
    
    /** Constructor.

        \param mgr 

        \param widget 

        \param global 
    */
    QSpinBoxSetting(PresetManager* mgr, QSpinBox* widget,
                    bool global = false);

    QSpinBox* duplicate(QWidget* parent);

private:
    QSpinBox* first_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
