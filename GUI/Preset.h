#ifndef SIDECAR_GUI_PRESET_H // -*- C++ -*-
#define SIDECAR_GUI_PRESET_H

#include "QtCore/QList"
#include "QtCore/QObject"
#include "QtCore/QString"
#include "QtCore/QVariant"

class QSettings;

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class PresetManager;
class Setting;

/** Collection of Setting values that represent a defined preset for an application. A Preset object only
    records the values that make up the preset. It relies on a PresetCollection object to do the actual
    manipulation of Setting object values when a Preset becomes active.
*/
class Preset : public QObject
{
    using Super = class QObject;
    Q_OBJECT
public:

    /** Obtain the Log device to use for Preset objects

        \return Log reference
    */
    static Logger::Log& Log();
    
    /** Constructor. Creates a new preset with a given name.

        \param name the name of the preset

        \param parent PresetManager owner of the preset
    */
    Preset(const QString& name, PresetManager* parent);

    /** Create a duplicate of this preset.

        \param name the name for the new Preset object

        \return new Preset object containing the same Setting values as this one
    */
    Preset* duplicate(const QString& name);

    /** Obtain the name of the preset.

        \return QString reference
    */
    const QString& getName() const { return name_; }

    bool isDirty() const { return dirty_; }

    /** Change the setting's name.

        \param name new name to use
    */
    void setName(const QString& name) { name_ = name; }

    /** Change the held preset value for a given Setting

        \param index which Setting value to change

        \param value new Setting value
    */
    void setValue(int index, const QVariant& value);

    void expand(int index, const QVariant& value);

    /** Save all held preset values to a QSettings object

        \param file the QSettings object to write into

        \param names list of Setting names to write
    */
    void save(QSettings& file, const QStringList& names);

    /** Restore all held preset values from a QSettings object

        \param file the QSettings object to read from

        \param settings list of Setting objects to update

	\param isActive if true, apply restored values to Setting objects
    */
    void restore(QSettings& file, const QList<Setting*>& settings,
                 bool isActive);

    /** Restore one preset value from a QSettings object

        \param file the QSettings object to read from

        \param setting the value to restore
    */
    void restoreOne(QSettings& file, Setting* setting);

    /** Apply held preset values to active Setting objects.

	\param settings list of Setting objects to update
    */
    void apply(const QList<Setting*>& settings);

private:

    PresetManager* getParent() const;

    QString name_;
    QList<QVariant> values_;
    bool dirty_;

    friend class PresetManager;
};

} // end namespace GUI
} // end namespace SideCar

#endif
