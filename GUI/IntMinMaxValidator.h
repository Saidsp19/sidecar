#ifndef SIDECAR_GUI_INTMINMAXVALIDATOR_H // -*- C++ -*-
#define SIDECAR_GUI_INTMINMAXVALIDATOR_H

#include "QtGui/QIntValidator"

class QSpinBox;

namespace SideCar {
namespace GUI {

class IntMinMaxValidator : public QIntValidator
{
    Q_OBJECT
    using Super = QIntValidator;
public:

    IntMinMaxValidator(QObject* parent, QSpinBox* min, QSpinBox* max);

    IntMinMaxValidator(QObject* parent, QSpinBox* min, QSpinBox* max,
                       int minMin, int maxMax);

private slots:

    void minChanged(int value);

    void maxChanged(int value);

private:
    QSpinBox* min_;
    QSpinBox* max_;
    int minMin_;
    int maxMax_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
