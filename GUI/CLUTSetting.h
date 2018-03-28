#ifndef SIDECAR_GUI_CLUTSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_CLUTSETTING_H

#include "GUI/CLUT.h"
#include "GUI/QComboBoxSetting.h"

namespace SideCar {
namespace GUI {

class CLUTSetting : public QComboBoxSetting {
    Q_OBJECT
    using Super = QComboBoxSetting;

public:
    /** Constructor

        \param mgr

        \param widget

        \param global
    */
    CLUTSetting(PresetManager* mgr, QComboBox* widget, bool global = false);

    /** Get value type.

        \return CLUT::Type
    */
    CLUT::Type getType() const { return CLUT::Type(getValue()); }

    /** Add Qt widget for editing this value.

        \param widget Qt widget to use for editing
    */
    void connectWidget(QComboBox* widget);
};

} // end namespace GUI
} // end namespace SideCar

#endif
