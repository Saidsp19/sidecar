#ifndef SIDECAR_GUI_SPECTRUM_SETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_SETTINGS_H

#include <cmath>

#include "GUI/ChannelSetting.h"
#include "GUI/Color.h"
#include "GUI/ColorButtonSetting.h"
#include "GUI/QCheckBoxSetting.h"
#include "GUI/QComboBoxSetting.h"
#include "GUI/QDoubleSpinBoxSetting.h"
#include "GUI/QRadioButtonSetting.h"
#include "GUI/QSpinBoxSetting.h"
#include "GUI/SettingsBlock.h"

namespace SideCar {
namespace GUI {
namespace Spectrum {

class Settings : public SettingsBlock {
    Q_OBJECT
    using Super = SettingsBlock;

public:
    enum DrawingMode { kPoints = 0, kLines };

    Settings(QDoubleSpinBoxSetting* sampleFrequencyValue, QComboBoxSetting* sampleFrequencyScale,
             ChannelSetting* videoChannel, ColorButtonSetting* color, QCheckBoxSetting* showGrid,
             QComboBoxSetting* drawingMode, QSpinBoxSetting* powerMax);

    double getSampleFrequency() const { return sampleFrequency_; }

    double getSampleFrequencyScaling() const { return sampleFrequencyScaling_; }

    ChannelSetting* getInputChannel() const { return inputChannel_; }

    int getPowerMax() const { return powerMax_->getValue(); }

    const QColor& getQColor() const { return qcolor_->getValue(); }

    const Color& getColor() const { return color_; }

    bool getShowGrid() const { return showGrid_->getValue(); }

    DrawingMode getDrawingMode() const { return DrawingMode(drawingMode_->getValue()); }

signals:

    void sampleFrequencyChanged(double sampleFrequency);

    void showGridChanged(bool newValue);

    void colorChanged(const QColor& color);

    void powerScalingChanged();

    void drawingModeChanged();

public slots:

    void setShowGrid(bool value) { showGrid_->setValue(value); }

private slots:

    void updateSampleFrequency();

    void handleColorChanged(const QColor& color);

private:
    QDoubleSpinBoxSetting* sampleFrequencyValue_;
    QComboBoxSetting* sampleFrequencyScale_;
    ChannelSetting* inputChannel_;
    ColorButtonSetting* qcolor_;
    QCheckBoxSetting* showGrid_;
    QComboBoxSetting* drawingMode_;
    QSpinBoxSetting* powerMax_;
    double sampleFrequency_;
    double sampleFrequencyScaling_;
    Color color_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
