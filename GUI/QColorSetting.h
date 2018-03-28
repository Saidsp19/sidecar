#ifndef SIDECAR_GUI_PPIWIDGET_COLORSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_PPIWIDGET_COLORSETTING_H

#include "QtGui/QColor"

#include "GUI/Setting.h"

namespace SideCar {
namespace GUI {

class ColorButtonWidget;

/** Derivation of the Setting class that works with QColor values.
 */
class QColorSetting : public Setting {
    Q_OBJECT
    using Super = Setting;

public:
    /** Constructor with initial default value

        \param mgr PresetManager object that records this setting

        \param name the name of the setting

        \param color initial value to use for the setting if one is not recorded

        \param global if true, this is a global setting, not associated with a
        specific Preset.
    */
    QColorSetting(PresetManager* mgr, const QString& name, const QColor& color, bool global = false);

    /** Obtain the current setting value.

        \return setting value
    */
    const QColor& getValue() const { return value_; }

    void connectWidget(ColorButtonWidget* widget);

signals:

    /** Notification sent out when the held value changes.

        \param color new held value
    */
    void valueChanged(const QColor& color);

public slots:

    /** Change the setting value. Emits the valueChanged() signal if the given value is different from the held
        one.

        \param color new value to use
    */
    void setValue(const QColor& color);

protected:
    /** Override of Setting method. Updates value_ attribute with new value and emits valueChanged() signal.
     */
    void valueUpdated();

private:
    QColor value_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
