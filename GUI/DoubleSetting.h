#ifndef SIDECAR_GUI_DOUBLESETTING_H // -*- C++ -*-
#define SIDECAR_GUI_DOUBLESETTING_H

#include "GUI/Setting.h"

class QDoubleSpinBox;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

/** Derivation of the Setting class that manages 'double' floating-point values.
 */
class DoubleSetting : public Setting {
    Q_OBJECT
    using Super = Setting;

public:
    /** Log device for DoubleSetting objects

        \return Log reference
    */
    static Logger::Log& Log();

    /** Constructor.

        \param mgr

        \param name

        \param global
    */
    DoubleSetting(PresetManager* mgr, const QString& name, bool global = false);

    /** Constructor.

        \param mgr

        \param name

        \param value

        \param global
    */
    DoubleSetting(PresetManager* mgr, const QString& name, double value, bool global = false);

    /** Obtain the current setting value.

        \return setting value
    */
    double getValue() const { return value_; }

    void connectWidget(QDoubleSpinBox* widget);

public slots:

    /** Change the setting value. Emits the valueChanged() signal if the given value is different than the held
        one.

        \param value new value to use
    */
    void setValue(double value);

signals:

    /** Notification sent out when the held value changes.

        \param value new held value
    */
    void valueChanged(double value);

protected:
    /** Override of Setting::valueUpdated() method. Records the new value and emits the valueChanged() signal.
     */
    void valueUpdated();

private:
    double value_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
