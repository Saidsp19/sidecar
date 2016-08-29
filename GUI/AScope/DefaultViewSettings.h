#ifndef SIDECAR_GUI_ASCOPE_DEFAULTVIEWSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_DEFAULTVIEWSETTINGS_H

#include "GUI/BoolSetting.h"
#include "GUI/DoubleSetting.h"
#include "GUI/IntSetting.h"
#include "GUI/SettingsBlock.h"

namespace SideCar {
namespace GUI {

namespace AScope {

class DefaultViewSettings : public SettingsBlock
{
public:

    DefaultViewSettings(IntSetting* sampleMin, IntSetting* sampleMax,
                        DoubleSetting* voltageMin, DoubleSetting* voltageMax,
                        BoolSetting* showPeakBars)
	: SettingsBlock(), sampleMin_(sampleMin), sampleMax_(sampleMax),
	  voltageMin_(voltageMin), voltageMax_(voltageMax),
	  showPeakBars_(showPeakBars) {}

    int getSampleMin() const { return sampleMin_->getValue(); }
    int getSampleMax() const { return sampleMax_->getValue(); }

    double getVoltageMin() const { return voltageMin_->getValue(); }
    double getVoltageMax() const { return voltageMax_->getValue(); }

    bool getVisible() const { return true; }
    bool getShowPeakBars() const { return showPeakBars_->getValue(); }

private:
    IntSetting* sampleMin_;
    IntSetting* sampleMax_;
    DoubleSetting* voltageMin_;
    DoubleSetting* voltageMax_;
    BoolSetting* showPeakBars_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

#endif
