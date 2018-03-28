#ifndef SIDECAR_GUI_TIMESETTING_H // -*- C++ -*-
#define SIDECAR_GUI_TIMESETTING_H

#include "QtCore/QTime"

#include "GUI/Setting.h"

class QTimeEdit;

namespace SideCar {
namespace GUI {

/** Derivation of the Setting class that manages QTime values (HH:MM:SS)
 */
class TimeSetting : public Setting {
    Q_OBJECT
    using Super = Setting;

public:
    /** Constructor.

        \param mgr

        \param name

        \param global
    */
    TimeSetting(PresetManager* mgr, const QString& name, bool global = false);

    /** Constructor.

        \param mgr

        \param name

        \param value

        \param global
    */
    TimeSetting(PresetManager* mgr, const QString& name, const QTime& value, bool global = false);

    /** Obtain the current setting value

        \return setting value
    */
    const QTime& getValue() const { return value_; }

    void connectWidget(QTimeEdit* widget);

public slots:

    /** Change the setting value. Emits the valueChanged() signal if the given value is different than the held
        one.

        \param value new value to use.
    */
    void setValue(const QTime& value);

signals:

    /** Notification sent out when the held value changes.

        \param value new held value
    */
    void valueChanged(const QTime& value);

protected:
    /** Override of Setting::valueUpdated() method. Records the new value and emits the valueChanged() signal.
     */
    void valueUpdated();

private:
    QTime value_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
