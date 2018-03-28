#ifndef SIDECAR_GUI_ASCOPE_CHANNELEDITOR_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_CHANNELEDITOR_H

#include "ui_ChannelEditor.h"

namespace SideCar {
namespace GUI {
namespace AScope {

/** Simple editor for changing the sample and voltage ranges of an input channel.
 */
class ChannelEditor : public QDialog, public Ui::ChannelEditor {
    Q_OBJECT
public:
    /** Constructor.

        \param parent
    */
    ChannelEditor(QWidget* parent = 0);
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

#endif
