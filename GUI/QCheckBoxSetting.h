#ifndef SIDECAR_GUI_QCHECKBOXSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_QCHECKBOXSETTING_H

#include "QtGui/QCheckBox"

#include "GUI/BoolSetting.h"

namespace SideCar {
namespace GUI {

/** Derivation of BoolSetting that works with a QCheckBox widget to display and change setting values. Uses the
    QCheckBox::toggled() signal to detect changes in the widget.
*/
class QCheckBoxSetting : public BoolSetting
{
public:
    
    /** Constructor.

        \param mgr 

        \param widget 

        \param global 
    */
    QCheckBoxSetting(PresetManager* mgr, QCheckBox* widget,
                     bool global = false);

    QCheckBox* duplicate(QWidget* parent = 0);

private:
    QCheckBox* first_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
