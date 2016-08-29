#ifndef SIDECAR_GUI_RANGETRUTHEMITTER_ADDRESSVALIDATOR_H // -*- C++ -*-
#define SIDECAR_GUI_RANGETRUTHEMITTER_ADDRESSVALIDATOR_H

#include "QtGui/QColor"
#include "QtGui/QRegExpValidator"

class QLineEdit;

namespace SideCar {
namespace GUI {
namespace RangeTruthEmitter {

class AddressValidator : public QRegExpValidator
{
    Q_OBJECT
    using Super = QRegExpValidator;
public:

    AddressValidator(QLineEdit* widget);

    State validate(QString& input, int& pos) const;

private:

    QLineEdit* widget_;
    QColor normalColor_;
};

} // end namespace RangeTruthEmitter
} // end namespace GUI
} // end namespace SideCar

#endif
