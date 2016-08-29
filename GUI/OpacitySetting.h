#ifndef SIDECAR_GUI_OPACITYSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_OPACITYSETTING_H

#include <cmath>

#include "GUI/DoubleSetting.h"

class QSlider;
class QSpinBox;

namespace Logger { class Log; };
namespace SideCar {
namespace GUI {

class OpacitySetting : public DoubleSetting
{
    Q_OBJECT
public:

    static Logger::Log& Log();

    static double FromWidget(int value) { return double(value) / 100.0; }

    static int ToWidget(double value) { return int(::rint(value * 100)); }

    OpacitySetting(PresetManager* mgr, QSpinBox* widget, bool global = false);

    void connectWidget(QSlider* widget);

    void connectWidget(QSpinBox* widget);

signals:

    void percentageChanged(int value);

private slots:

    void widgetChanged(int value);

    void sliderChanged(int value);

private:

    void valueUpdated();
};

} // end namespace GUI
} // end namespace SideCar

#endif
