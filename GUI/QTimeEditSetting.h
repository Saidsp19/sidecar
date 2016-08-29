#ifndef SIDECAR_GUI_QTIMEEDITSETTING // -*- C++ -*-
#define SIDECAR_GUI_QTIMEEDITSETTING

#include "QtGui/QTimeEdit"

#include "GUI/TimeSetting.h"

namespace SideCar {
namespace GUI {

/** Derivation of BoolSetting that works with a QCheckBox widget to display and change setting values. Uses the
    QCheckBox::toggled() signal to detect changes in the widget.
*/
class QTimeEditSetting : public TimeSetting
{
    Q_OBJECT
    using Super = TimeSetting;
public:
    
    /** Constructor.

        \param mgr 

        \param widget 

        \param global 
    */
    QTimeEditSetting(PresetManager* mgr, QTimeEdit* widget,
                     bool global = false);
};

} // end namespace GUI
} // end namespace SideCar

#endif
