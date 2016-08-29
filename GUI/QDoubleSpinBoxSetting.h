#ifndef SIDECAR_GUI_QDOUBLESPINBOXSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_QDOUBLESPINBOXSETTING_H

#include "GUI/DoubleSetting.h"

class QDoubleSpinBox;

namespace SideCar {
namespace GUI {

/** Derivation of DoubleSetting that works with a QDoubleSpinBox widget to display and change setting values.
    Uses the QDoubleSpinBox::editingFinished() signal to detect changes in the widget.
*/
class QDoubleSpinBoxSetting : public DoubleSetting
{
    Q_OBJECT
public:

    /** Constructor.

        \param mgr 

        \param widget 

        \param global 
    */
    QDoubleSpinBoxSetting(PresetManager* mgr, QDoubleSpinBox* widget,
                          bool global = false);

    QDoubleSpinBox* duplicate(QWidget* parent = 0);

private:
    QDoubleSpinBox* first_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
