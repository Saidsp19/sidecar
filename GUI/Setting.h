#ifndef SIDECAR_GUI_SETTING_H // -*- C++ -*-
#define SIDECAR_GUI_SETTING_H

#include "QtCore/QObject"
#include "QtCore/QString"
#include "QtCore/QVariant"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class PresetManager;

/** A general name/value object that holds QVariant values. Changing the object's value emits the
    opaqueValueChanged() signal, and possibly the save() signal depending on the method used to invoke the
    change, restoreValue() or setValue().

    The PresetManager class manages one or more Setting objects along with a
    set of preset values.
*/
class Setting : public QObject {
    using Super = class QObject;
    Q_OBJECT
public:
    /** Log device for Setting objects

        \return Log reference
    */
    static Logger::Log& Log();

    /** Constructor without a default setting value

        \param mgr the PrestMangager object that manages the Setting

        \param name the name of the Setting object

        \param global if true, register setting as a global setting
    */
    Setting(PresetManager* mgr, const QString& name, bool global = false);

    /** Constructor with a default setting value

        \param mgr the PrestMangager object that manages the Setting

        \param name the name of the Setting object

        \param value the initial value of the Setting object

        \param global if true, register setting as a global setting
    */
    Setting(PresetManager* mgr, const QString& name, const QVariant& value, bool global = false);

    /** Obtain the name of the Setting object

        \return Setting name
    */
    const QString& getName() const { return name_; }

    /** Obtain the current value of the Setting object

        \return read-only value reference
    */
    const QVariant& getOpaqueValue() const { return value_; }

    /** Set the Setting's value and emit the opaqueValueChanged() signal followed by the save() signal.

        \param value
    */
    void setOpaqueValue(const QVariant& value);

    bool isEnabled() const { return enabled_; }

signals:

    /** Notification sent out by the Setting object when the held value changes due to the setValue() method. If
        the Setting object has been registered with a PresetManager object using the PresetManager::addSetting()
        or PresetManager::addGlobal() method, then this signal will invoke the PresetManager::settingSave() or
        PresetManager::globalSave() method respectively.

        \param index value assigned by PresetManager

        \param value new Setting value
    */
    void save(int index, const QVariant& value);

    /** Notification sent out by the Setting object when its enabled state changes.

        \param state new state
    */
    void enabledChanged(bool state);

    void valueChanged();

public slots:

    void setEnabled(bool state);

protected:
    /** Hook method used to detect when the internal value changes. Derived classes should override to perform
        actions when the value changes.
    */
    virtual void valueUpdated() {}

    void resetOpaqueValue() { value_ = QVariant(); }

    void connectWidget(QWidget* widget);

    void disconnectWidget(QWidget* widget);

private:
    /** Set the Setting's value and emit the opaqueValueChanged() signal. This method is only used by Preset and
        PresetManager objects when restoring a Setting value from a QSetting object.

        \param value the new value to hold
    */
    void restoreValue(const QVariant& value);

    QString name_;
    QVariant value_;
    int index_;
    bool enabled_;

    friend class Preset;
    friend class PresetManager;
};

} // end namespace GUI
} // end namespace SideCar

#endif
