#ifndef SIDECAR_GUI_SPECTRUM_AZIMUTHLATCH_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_AZIMUTHLATCH_H

#include "GUI/OnOffSettingsBlock.h"
#include "GUI/QCheckBoxSetting.h"
#include "Utils/Utils.h"

#include "ui_AzimuthLatch.h"

namespace SideCar {
namespace GUI {

class PresetManager;

namespace Spectrum {

class AzimuthLatch : public QWidget, private Ui_AzimuthLatch {
    Q_OBJECT
    using Super = QWidget;

public:
    AzimuthLatch(QWidget* parent = 0);

    bool check(double radians);

private slots:

    void handleEnabledChanged(bool state);

private:
    void updateCaughtIndicator();

    double lastAzimuth_;
    bool caught_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
