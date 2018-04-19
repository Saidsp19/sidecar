#ifndef SIDECAR_GUI_RANGETRUTHEMITTER_ADDRESSLINEEDIT_H // -*- C++ -*-
#define SIDECAR_GUI_RANGETRUTHEMITTER_ADDRESSLINEEDIT_H

#include "QtWidgets/QLineEdit"

namespace SideCar {
namespace GUI {
namespace RangeTruthEmitter {

class AddressLineEdit : public QLineEdit {
    Q_OBJECT
    using Super = QLineEdit;

public:
    AddressLineEdit(QWidget* parent) : Super(parent) {}

private:
    void keyPressEvent(QKeyEvent* event);
};

} // end namespace RangeTruthEmitter
} // end namespace GUI
} // end namespace SideCar

#endif
