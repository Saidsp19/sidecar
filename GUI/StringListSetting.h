#ifndef SIDECAR_GUI_STRINGLISTSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_STRINGLISTSETTING_H

#include "QtCore/QStringList"

#include "GUI/Setting.h"

namespace SideCar {
namespace GUI {

/** Derivation of the Setting class that manages boolean values.
 */
class StringListSetting : public Setting {
    Q_OBJECT
    using Super = Setting;

public:
    /** Constructor.

        \param mgr

        \param name

        \param global
    */
    StringListSetting(PresetManager* mgr, const QString& name, bool global = false);

    /** Constructor.

        \param mgr

        \param name

        \param value

        \param global
    */
    StringListSetting(PresetManager* mgr, const QString& name, const QStringList& value, bool global = false);

    /** Obtain the current setting value.

        \return setting value
    */
    const QStringList& getValue() const { return value_; }

public slots:

    /** Change the setting value. Emits the valueChanged() signal if the given value is diferent than the held
        one.

        \param value new value to use
    */
    void setValue(const QStringList& value);

signals:

    /** Notification sent out when the held value changes.

        \param value new held value
    */
    void valueChanged(const QStringList& value);

protected:
    /** Override of Setting::valueUpdated() method. Records the new value and emits the valueChanged() signal.
     */
    void valueUpdated();

private:
    QStringList value_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
