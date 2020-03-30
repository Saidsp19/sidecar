#include "QtCore/QSettings"
#include "QtGui/QInputEvent"
#include "QtWidgets/QApplication"
#include "QtWidgets/QInputDialog"

#include "GUI/Utils.h"
#include "Utils/Utils.h"

#include "GeneratorConfiguration.h"

#include "ui_GeneratorConfiguration.h"

using namespace SideCar::GUI::SignalGenerator;

GeneratorConfiguration::GeneratorConfiguration(double sampleFrequency) :
    Super(), gui_(new Ui::GeneratorConfiguration), sampleFrequency_(sampleFrequency), amplitude_(0.0),
    phaseOffset_(1.0), counter_(0), enabled_(true)
{
    gui_->setupUi(this);
    on_frequency__valueChanged(gui_->frequency_->value());
    on_amplitude__valueChanged(gui_->amplitude_->value());
    on_phaseOffset__valueChanged(gui_->phaseOffset_->value());
    dcOffset_ = gui_->dcOffset_->value();
    complexValueType_ = ComplexValueType(gui_->complexValue_->currentIndex());
    initialize();
}

GeneratorConfiguration::GeneratorConfiguration(GeneratorConfiguration* basis) :
    Super(), gui_(new Ui::GeneratorConfiguration), sampleFrequency_(basis->sampleFrequency_),
    amplitude_(basis->amplitude_), phaseOffset_(basis->phaseOffset_), dcOffset_(basis->dcOffset_),
    complexValueType_(basis->complexValueType_), counter_(0), enabled_(true)
{
    gui_->setupUi(this);
    gui_->frequency_->setValue(basis->gui_->frequency_->value());
    gui_->frequencyScale_->setCurrentIndex(basis->gui_->frequencyScale_->currentIndex());
    gui_->amplitude_->setValue(basis->gui_->amplitude_->value());
    gui_->amplitudeValue_->setText(basis->gui_->amplitudeValue_->text());
    gui_->phaseOffset_->setValue(basis->gui_->phaseOffset_->value());
    gui_->phaseOffsetValue_->setText(basis->gui_->phaseOffsetValue_->text());
    gui_->dcOffset_->setValue(dcOffset_);
    gui_->complexValue_->setCurrentIndex(complexValueType_);
    gui_->enabled_->setChecked(true);
    initialize();
}

void
GeneratorConfiguration::initialize()
{
    gui_->frequency_->installEventFilter(this);
    gui_->amplitude_->installEventFilter(this);
    gui_->phaseOffset_->installEventFilter(this);
    calculateRadiansPerSample();
    connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), SLOT(focusChanged(QWidget*, QWidget*)));
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Window);
}

void
GeneratorConfiguration::restoreFromSettings(QSettings& settings)
{
    gui_->frequency_->setValue(settings.value("Frequency").toInt());
    gui_->frequencyScale_->setCurrentIndex(settings.value("FrequencyScale").toInt());
    gui_->amplitude_->setValue(settings.value("Amplitude").toInt());
    gui_->phaseOffset_->setValue(settings.value("PhaseOffset").toInt());
    gui_->dcOffset_->setValue(settings.value("DCOffset").toDouble());
    gui_->complexValue_->setCurrentIndex(settings.value("ComplexValue").toInt());
    gui_->enabled_->setChecked(settings.value("Enabled", true).toBool());
}

void
GeneratorConfiguration::saveToSettings(QSettings& settings) const
{
    settings.setValue("Frequency", gui_->frequency_->value());
    settings.setValue("FrequencyScale", gui_->frequencyScale_->currentIndex());
    settings.setValue("Amplitude", gui_->amplitude_->value());
    settings.setValue("PhaseOffset", gui_->phaseOffset_->value());
    settings.setValue("DCOffset", gui_->dcOffset_->value());
    settings.setValue("ComplexValue", gui_->complexValue_->currentIndex());
    settings.setValue("Enabled", gui_->enabled_->isChecked());
}

double
GeneratorConfiguration::getNextValue()
{
    return ::sin(radiansPerSample_ * counter_++ + phaseOffset_) * amplitude_ + dcOffset_;
}

void
GeneratorConfiguration::normalAddTo(AmplitudeVector& buffer)
{
    AmplitudeVector::iterator pos = buffer.begin();
    AmplitudeVector::iterator end = buffer.end();
    while (pos < end) { *pos++ += getNextValue(); }
}

void
GeneratorConfiguration::complexAddTo(AmplitudeVector& buffer)
{
    AmplitudeVector::iterator pos = buffer.begin();
    AmplitudeVector::iterator end = buffer.end();
    if (complexValueType_ == kIandQ) {
        while (pos < end) {
            double value = getNextValue();
            *pos++ += value;
            *pos++ += value;
        }
    } else {
        if (complexValueType_ == kQ) ++pos;
        while (pos < end) {
            *pos += getNextValue();
            pos += 2;
        }
    }
}

void
GeneratorConfiguration::setSampleFrequency(double sampleFrequency)
{
    sampleFrequency_ = sampleFrequency;
    calculateRadiansPerSample();
}

void
GeneratorConfiguration::calculateRadiansPerSample()
{
    double samplesPerCycle = sampleFrequency_ / getSignalFrequency();
    radiansPerSample_ = M_PI * 2.0 / samplesPerCycle;
}

void
GeneratorConfiguration::notify()
{
    if (enabled_) emit configurationChanged();
}

void
GeneratorConfiguration::on_frequency__valueChanged(int value)
{
    gui_->frequencyValue_->setNum(value);
    calculateRadiansPerSample();
    notify();
}

void
GeneratorConfiguration::on_frequencyScale__currentIndexChanged(int value)
{
    calculateRadiansPerSample();
    notify();
}

void
GeneratorConfiguration::on_amplitude__valueChanged(int value)
{
    double amplitude = double(value) / 100.0;
    gui_->amplitudeValue_->setNum(amplitude);
    if (amplitude_ != amplitude) {
        amplitude_ = amplitude;
        notify();
    }
}

void
GeneratorConfiguration::on_phaseOffset__valueChanged(int value)
{
    double phaseOffset = double(value) / 100.0 * M_PI * 2.0;
    gui_->phaseOffsetValue_->setText(
        QString::number(Utils::radiansToDegrees(phaseOffset), 'f', 1).append(GUI::DegreeSymbol()));
    if (phaseOffset_ != phaseOffset) {
        phaseOffset_ = phaseOffset;
        notify();
    }
}

void
GeneratorConfiguration::on_dcOffset__valueUpdated(double dcOffset)
{
    if (dcOffset_ != dcOffset) {
        dcOffset_ = dcOffset;
        notify();
    }
}

void
GeneratorConfiguration::on_complexValue__currentIndexChanged(int complexValueType)
{
    if (complexValueType_ != ComplexValueType(complexValueType)) {
        complexValueType_ = ComplexValueType(complexValueType);
        notify();
    }
}

void
GeneratorConfiguration::on_enabled__toggled(bool state)
{
    if (enabled_ != state) {
        enabled_ = state;
        emit configurationChanged();
    }
}

double
GeneratorConfiguration::getSignalFrequency() const
{
    return ::pow(10.0, gui_->frequencyScale_->currentIndex() * 3.0) * gui_->frequency_->value();
}

void
GeneratorConfiguration::setSelected(bool selected)
{
    auto windowColor = QGuiApplication::palette().window().color();
    auto p(palette());
    p.setColor(QPalette::Active, QPalette::Window, selected ? windowColor.lighter() : windowColor);
    setPalette(p);
    update();
}

void
GeneratorConfiguration::mousePressEvent(QMouseEvent* event)
{
    emit activeConfiguration(this);
}

void
GeneratorConfiguration::focusChanged(QWidget* old, QWidget* now)
{
    if (!isAncestorOf(old) && isAncestorOf(now)) { emit activeConfiguration(this); }
}

bool
GeneratorConfiguration::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        QInputEvent* inputEvent = static_cast<QInputEvent*>(event);

        if (inputEvent->modifiers() & Qt::ControlModifier) {
            if (event->type() == QEvent::MouseButtonPress) return true;
            QDial* w = qobject_cast<QDial*>(obj);
            if (w->objectName() == "frequency_") {
                int value = w->value();
                bool ok = false;
                value = QInputDialog::getInt(w, "Frequency", "Frequency", value, 1, 999, 10, &ok);
                if (ok) { w->setValue(value); }
            } else if (w->objectName() == "amplitude_") {
                double value = w->value() / 100.0;
                bool ok = false;
                value = QInputDialog::getDouble(w, "Amplitude", "Amplitude", value, 0.0, 1.0, 2, &ok);
                if (ok) { w->setValue(int(::rint(value * 100))); }
            } else if (w->objectName() == "phaseOffset_") {
                double value = w->value() / 50.0 * 180.0;
                bool ok = false;
                value = QInputDialog::getDouble(w, "Phase", "Phase", value, -180.0, 180.0, 2, &ok);
                if (ok) { w->setValue(int(::rint(value / 180.0 * 50.0))); }
            }
            return true;
        }
    }

    return false;
}
