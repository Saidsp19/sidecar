#ifndef SIDECAR_GUI_INTSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_INTSETTING_H

#include "GUI/Setting.h"

class QComboBox;
class QSpinBox;

namespace SideCar {
namespace GUI {

/** Derivation of the Setting class that manages integer values.
 */
class IntSetting : public Setting
{
    Q_OBJECT
    using Super = Setting;
public:
    
    /** Constructor with no default value.

        \param mgr PresetManager object that records this setting

        \param name the name of the setting

        \param global if true, this is a global setting, not associated with a
        specific Preset.
    */
    IntSetting(PresetManager* mgr, const QString& name, bool global = false);

    /** Constructor with initial default value.

        \param mgr PresetManager object that records this setting

        \param name the name of the setting

	\param value initial value to use for the setting if one is not recorded

        \param global if true, this is a global setting, not associated with a
        specific Preset.
    */
    IntSetting(PresetManager* mgr, const QString& name, int value,
               bool global = false);

    /** Obtain the current setting value

        \return setting value
    */
    int getValue() const { return value_; }

    void connectWidget(QSpinBox* widget);

    void connectWidget(QComboBox* widget);

public slots:

    /** Change the setting value. Emits the valueChanged() signal if the given value is different than the held
        one.

        \param value new value to use.
    */
    virtual void setValue(int value);

signals:

    /** Notification sent out when the held value changes.

        \param value new held value
    */
    void valueChanged(int value);

protected:

    /** Override of Setting::valueUpdated() method. Records the new value and emits the valueChanged() signal.
     */
    void valueUpdated();

private:

    int value_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
