#include <cmath>

#include "QtWidgets/QDoubleSpinBox"

#include "DoubleMinMaxValidator.h"

using namespace SideCar::GUI;

DoubleMinMaxValidator::DoubleMinMaxValidator(QObject* parent, QDoubleSpinBox* min, QDoubleSpinBox* max) :
    Super(min->minimum(), max->maximum(), min->decimals(), parent), min_(min), max_(max), minMin_(min->minimum()),
    maxMax_(max->maximum())
{
    epsilon_ = ::pow(10, -min->decimals());
    connect(min_, SIGNAL(valueChanged(double)), SLOT(minChanged(double)));
    connect(max_, SIGNAL(valueChanged(double)), SLOT(maxChanged(double)));
    min_->setMaximum(maxMax_ - epsilon_);
    max_->setMinimum(minMin_ + epsilon_);
}

DoubleMinMaxValidator::DoubleMinMaxValidator(QObject* parent, QDoubleSpinBox* min, QDoubleSpinBox* max,
                                             double minMin, double maxMax) :
    Super(min->minimum(), max->maximum(), min->decimals(), parent),
    min_(min), max_(max), minMin_(minMin), maxMax_(maxMax)
{
    epsilon_ = ::pow(10, -min->decimals());
    connect(min_, SIGNAL(valueChanged(double)), SLOT(minChanged(double)));
    connect(max_, SIGNAL(valueChanged(double)), SLOT(maxChanged(double)));
    min_->setRange(minMin_, maxMax_ - epsilon_);
    max_->setRange(minMin_ + epsilon_, maxMax_);
}

void
DoubleMinMaxValidator::minChanged(double value)
{
    if (max_->value() <= value) { max_->setValue(value + epsilon_); }
}

void
DoubleMinMaxValidator::maxChanged(double value)
{
    if (min_->value() >= value) { min_->setValue(value - epsilon_); }
}
