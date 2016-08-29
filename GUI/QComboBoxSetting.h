#ifndef SIDECAR_GUI_QCOMBOBOXSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_QCOMBOBOXSETTING_H

#include "QtGui/QComboBox"

#include "GUI/IntSetting.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

/** Derivation of StringSetting that works with a QComboBox widget to display and change setting values. Uses
    the QComboBox::activated() signal to detect changes in the widget.
*/
class QComboBoxSetting : public IntSetting
{
    Q_OBJECT
    using Super = IntSetting;
public:

    /** 

        \return 
    */
    static Logger::Log& Log();

    /** Constructor.

        \param mgr 

        \param widget 

        \param global 
    */
    QComboBoxSetting(PresetManager* mgr, QComboBox* widget,
                     bool global = false);

    /** 

        \return 
    */
    QComboBox* getWidget() const { return first_; }

    /** 

        \param parent 

        \return 
    */
    QComboBox* duplicate(QWidget* parent = 0);

    /** 

        \param widget 
    */
    void connectWidget(QComboBox* widget);

private:
    QComboBox* first_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
