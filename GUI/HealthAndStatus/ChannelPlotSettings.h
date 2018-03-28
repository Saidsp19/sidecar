#ifndef SIDECAR_GUI_HEALTHANDSTATUS_SETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_HEALTHANDSTATUS_SETTINGS_H

#include "GUI/DoubleSetting.h"
#include "GUI/IntSetting.h"
#include "GUI/QColorSetting.h"
#include "GUI/SettingsBlock.h"

class QDialog;

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {
namespace HealthAndStatus {

class ChannelPlotWidget;
class ConfigurationWindow;

class ChannelPlotSettings : public SettingsBlock {
    Q_OBJECT
    using Super = SettingsBlock;

public:
    static Logger::Log& Log();

    ChannelPlotSettings(PresetManager* presetManager, QObject* parent, const QString& name);

    const QString& getName() const { return name_; }

    void connectToWidget(ChannelPlotWidget* widget);

    const QColor& getSamplesColor() const { return samplesColor_.getValue(); }

    int getAlgorithm() const { return algorithm_.getValue(); }

    int getRunningMedianWindowSize() const { return runningMedianWindowSize_.getValue(); }

    int getMessageDecimation() const { return messageDecimation_.getValue(); }

    int getSampleStartIndex() const { return sampleStartIndex_.getValue(); }

    int getSampleCount() const { return sampleCount_.getValue(); }

    double getExpectedVariance() const { return expectedVariance_.getValue(); }

    double getExpected() const { return expected_.getValue(); }

    void connectToEditor(ConfigurationWindow* gui);

    void disconnectFromEditor(ConfigurationWindow* gui);

    void copyFrom(ChannelPlotSettings* settings);

    double getCurrentValue() const;

private:
    QString name_;
    IntSetting messageDecimation_;
    IntSetting sampleStartIndex_;
    IntSetting sampleCount_;
    IntSetting runningMedianWindowSize_;
    IntSetting algorithm_;
    QColorSetting samplesColor_;
    DoubleSetting expectedVariance_;
    DoubleSetting expected_;
    ChannelPlotWidget* widget_;
};

} // end namespace HealthAndStatus
} // end namespace GUI
} // end namespace SideCar

#endif
