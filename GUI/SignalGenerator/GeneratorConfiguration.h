#ifndef SIDECAR_GUI_SIGNALGENERATOR_GENERATORCONFIGURATION_H // -*- C++ -*-
#define SIDECAR_GUI_SIGNALGENERATOR_GENERATORCONFIGURATION_H

#include <cmath>
#include <vector>

#include "QtGui/QWidget"

class QSettings;

namespace Ui {
class GeneratorConfiguration;
}

namespace SideCar {
namespace GUI {
namespace SignalGenerator {

class GeneratorConfiguration : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    using AmplitudeVector = std::vector<double>;

    enum ComplexValueType { kIandQ, kI, kQ };

    GeneratorConfiguration(double sampleFrequency);

    GeneratorConfiguration(GeneratorConfiguration* basis);

    double getSignalFrequency() const;

    bool isEnabled() const { return enabled_; }

    void saveToSettings(QSettings& settings) const;

    void restoreFromSettings(QSettings& settings);

    bool eventFilter(QObject* obj, QEvent* event);

signals:

    void activeConfiguration(GeneratorConfiguration* config);

    void configurationChanged();

public slots:

    void normalAddTo(AmplitudeVector& buffer);

    void complexAddTo(AmplitudeVector& buffer);

    void setSampleFrequency(double sampleFrequency);

    void setSelected(bool state);

    void reset() { counter_ = 0; }

private slots:

    void on_frequency__valueChanged(int value);

    void on_frequencyScale__currentIndexChanged(int index);

    void on_amplitude__valueChanged(int value);

    void on_phaseOffset__valueChanged(int value);

    void on_dcOffset__valueUpdated(double value);

    void on_complexValue__currentIndexChanged(int value);

    void on_enabled__toggled(bool state);

    void focusChanged(QWidget* old, QWidget* now);

private:
    void initialize();

    double getNextValue();

    void calculateRadiansPerSample();

    void mousePressEvent(QMouseEvent* event);

    void notify();

    Ui::GeneratorConfiguration* gui_;
    double sampleFrequency_;
    double amplitude_;
    double phaseOffset_;
    double dcOffset_;
    double radiansPerSample_;
    ComplexValueType complexValueType_;
    size_t counter_;
    bool enabled_;
    QColor selected_;
    QColor unselected_;
};

} // end namespace SignalGenerator
} // end namespace GUI
} // end namespace SideCar

#endif
