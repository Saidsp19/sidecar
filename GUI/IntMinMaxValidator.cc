#include "QtWidgets/QSpinBox"

#include "IntMinMaxValidator.h"

using namespace SideCar::GUI;

IntMinMaxValidator::IntMinMaxValidator(QObject* parent, QSpinBox* min, QSpinBox* max) :
    Super(min->minimum(), max->maximum(), parent), min_(min), max_(max), minMin_(min->minimum()),
    maxMax_(max->maximum())
{
    connect(min_, SIGNAL(valueChanged(int)), SLOT(minChanged(int)));
    connect(max_, SIGNAL(valueChanged(int)), SLOT(maxChanged(int)));
    min_->setMaximum(maxMax_ - 1);
    max_->setMinimum(minMin_ + 1);
}

IntMinMaxValidator::IntMinMaxValidator(QObject* parent, QSpinBox* min, QSpinBox* max, int minMin, int maxMax) :
    Super(min->minimum(), max->maximum(), parent), min_(min), max_(max), minMin_(minMin), maxMax_(maxMax)
{
    connect(min_, SIGNAL(valueChanged(int)), SLOT(minChanged(int)));
    connect(max_, SIGNAL(valueChanged(int)), SLOT(maxChanged(int)));
    min_->setRange(minMin_, maxMax_ - 1);
    max_->setRange(minMin_ + 1, maxMax_);
}

void
IntMinMaxValidator::minChanged(int value)
{
    if (max_->value() <= value) max_->setValue(value + 1);
}

void
IntMinMaxValidator::maxChanged(int value)
{
    if (min_->value() >= value) min_->setValue(value - 1);
}
