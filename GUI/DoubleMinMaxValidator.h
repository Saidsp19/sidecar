#ifndef SIDECAR_GUI_DOUBLEMINMAXVALIDATOR_H // -*- C++ -*-
#define SIDECAR_GUI_DOUBLEMINMAXVALIDATOR_H

#include "QtGui/QDoubleValidator"

class QDoubleSpinBox;

namespace SideCar {
namespace GUI {

class DoubleMinMaxValidator : public QDoubleValidator
{
    Q_OBJECT
    using Super = QDoubleValidator;
public:

    DoubleMinMaxValidator(QObject* parent, QDoubleSpinBox* min, QDoubleSpinBox* max);

    DoubleMinMaxValidator(QObject* parent, QDoubleSpinBox* min, QDoubleSpinBox* max, double minMin,
                          double minMax);

private slots:

    void minChanged(double value);

    void maxChanged(double value);

private:
    QDoubleSpinBox* min_;
    QDoubleSpinBox* max_;
    double epsilon_;
    double minMin_;
    double maxMax_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
