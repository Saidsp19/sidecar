#ifndef SIDECAR_GUI_SPECTRUM_FFTSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_FFTSETTINGS_H

#include "GUI/QCheckBoxSetting.h"
#include "GUI/QComboBoxSetting.h"
#include "GUI/QSpinBoxSetting.h"
#include "GUI/SettingsBlock.h"

namespace SideCar {
namespace GUI {
namespace Spectrum {

class Settings;

class FFTSettings : public SettingsBlock {
    Q_OBJECT
    using Super = SettingsBlock;

public:
    FFTSettings(QComboBoxSetting* fftSizePower, QSpinBoxSetting* gateStart, QComboBoxSetting* windowType,
                QCheckBoxSetting* zeroPad, QSpinBoxSetting* workerThreadCount, QSpinBoxSetting* smoothing,
                Settings* settings);

    double getFrequencyMin() const { return frequencyMin_; }

    double getFrequencyStep() const { return frequencyStep_; }

    int getFFTSize() const { return fftSize_; }

    int getGateStart() const { return gateStart_->getValue(); }

    int getWindowType() const { return windowType_->getValue(); }

    int getInputSmoothing() const { return smoothing_->getValue(); }

    int getWorkerThreadCount() const { return workerThreadCount_->getValue(); }

    bool getZeroPad() const { return zeroPad_->getValue(); }

    QComboBox* duplicateFFTSizePower(QWidget* parent = 0) const { return fftSizePower_->duplicate(parent); }

    QComboBox* duplicateWindowType(QWidget* parent = 0) const { return windowType_->duplicate(parent); }

signals:

    void gateStartChanged(int newValue);

    void windowTypeChanged(int newValue);

    void fftSizeChanged(int newValue);

    void frequencyStepChanged(double newValue);

    void inputSmoothingChanged(int newValue);

    void workerThreadCountChanged(int newValue);

    void zeroPadChanged(bool newValue);

private slots:

    void updateFFTSize();

    void sampleFrequencyChanged(double sampleFrequency);

private:
    QComboBoxSetting* fftSizePower_;
    QSpinBoxSetting* gateStart_;
    QComboBoxSetting* windowType_;
    QCheckBoxSetting* zeroPad_;
    QSpinBoxSetting* workerThreadCount_;
    QSpinBoxSetting* smoothing_;
    int fftSize_;
    double frequencyMin_;
    double frequencyStep_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
