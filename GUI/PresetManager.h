#ifndef SIDECAR_GUI_PRESETMANAGER_H // -*- C++ -*-
#define SIDECAR_GUI_PRESETMANAGER_H

#include "QtCore/QList"
#include "QtCore/QObject"
#include "QtCore/QSettings"
#include "QtCore/QString"
#include "QtCore/QStringList"
#include "QtCore/QVariant"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class Preset;
class Setting;

/** Manager for Preset objects and associated Setting objects. A Preset contains a set of values for a
    collection of Setting objects. When a Setting object changes due to a GUI widget change or programatically,
    the PresetManager saves the new setting value in a QSettings file. When the active Preset object changes due
    to a call to applyPreset(), all Setting objects receive new values from the active Preset object.
*/
class PresetManager : public QObject
{
    using Super = QObject;
    Q_OBJECT
public:

    static Logger::Log& Log();

    /** Constructor.

        \param prefix name to use for this collection of presets.

        \param parent optional parent used for Qt automated destruction
    */
    PresetManager(const QString& prefix, QObject* parent = 0);

    /** Add a Setting object to the list of preset values. A preset Setting object has values stored in Preset
        objects, and thus they may change values when the active Preset object changes.

        \param setting the Setting object to register

	\param global if true, register as a global setting

        \return internal index of the Setting object that may be used in a
        later getSetting() call.
    */
    size_t addSetting(Setting* setting, bool global = false);

    virtual QVariant upgradeSetting(const QString& name,
                                    const QVariant& value)
	{ return value; }

    /** Obtain a preset Setting object at a given index

        \param index the index of the Setting object to return

        \return Setting object
    */
    Setting* getSetting(int index) const
	{ return settings_[index]; }

    /** Obtain a preset Setting object with a given name.

        \param name 

        \return 
    */
    Setting* getSetting(const QString& name) const;

    /** Obtain a global Setting object at a given index

        \param index the index of the Setting object to return

        \return Setting object
    */
    Setting* getGlobal(int index) const
	{ return globals_[index]; }

    Setting* getGlobal(const QString& name) const;

    Preset* getPreset(const QString& name) const;

    bool isRestoring() const { return restoring_; }

    /** Save the values associated with a specific preset.

        \param index which preset to save
    */
    virtual void savePreset(int index);

    /** Restore the values associated with a specific preset.

        \param index which preset to restore
    */
    virtual void restorePreset(int index);

    /** Create a new Preset object, duplicated from the active Preset object, and save it.

        \param name name of the new Preset object
    */
    virtual bool addPreset(const QString& name);

    /** Remove an existing Preset object. If the Preset object is the active one, the changes the active Preset
        object to the default one.

        \param index the index of the Preset object to remove
    */
    virtual bool removePreset(int index);

    /** Obtain a list of names of defined Preset objects

        \return QString list
    */
    QStringList getPresetNames() const;

    /** Obtain the name of a particular Preset object.

        \param index which preset to work with

        \return Preset name
    */
    const QString& getPresetName(int index) const;

    /** Obtain the index of the active preset.

        \return 
    */
    int getActivePresetIndex() const { return activePresetIndex_; }

    /** Determine if a given preset has changed since it was last saved to disk.

        \param index which index to check

        \return true if so
	\sa getAnyIsDirty()
    */
    bool getPresetIsDirty(int index) const;

    /** Determine if any preset has changed since it was last saved to disk.

        \return true if so
	\sa getPresetIsDirty()
    */
    bool getAnyIsDirty() const;

    /** Determine if the active preset is 'dirty', containing a Settting object with a value different than what
        is saved in the QSettings file.

        \return if so
	\sa getPresetIsDirty(), getAnyIsDirty()
    */
    bool getActiveIsDirty() const
	{ return getPresetIsDirty(activePresetIndex_); }

    virtual int getCurrentVersion() const { return 1; }

    int getSavedVersion() const { return savedVersion_; }

    bool isOldVersion() const { return savedVersion_ != getCurrentVersion(); }

signals:

    /** Notification sent out when a preset's dirty state changes.

        \param index which preset changed

        \param isDirty true if the preset is dirty, false if not
    */
    void presetDirtyStateChanged(int index, bool isDirty);

    /** Notification sent out when the active preset changes

        \param index which preset is active
    */
    void activePresetChanged(int index);

    /** Notification sent out when the list of preset names changes due to preset addition, removal, or name
        change.

        \param names the list of current preset names
    */
    void presetNamesChanged(const QStringList& names);

public slots:

    /** Make an indicated Preset object active, and apply its values to registered Setting objects.

        \param index the index of the Preset object to make active
    */
    virtual bool applyPreset(int index);

    /** Restore preset settings and global values from a QSettings file.
     */
    virtual void restoreAllPresets();

    /** Save all preset settings and global values to a QSettings file.
     */
    virtual void saveAllPresets(bool saveChanges = true);

protected slots:

    /** Save one preset Setting object to a QSettings file

        \param index the index of the Setting object to save

        \param value the new value of the Setting object
    */
    void settingSave(int index, const QVariant& value);

    /** Save one global Setting object to a QSettings file

        \param index the index of the Setting object to save

        \param value the new value of the Setting object
    */
    void globalSave(int index, const QVariant& value);

protected:

private:

    /** Restore a given global Setting object. Invoked whenever addSetting() executes after restoreAllPresets().

        \param setting the global Setting object to restore
    */
    void globalRestore(Setting* setting);

    /** Restore a given non-global Setting object. Invoked whenever addSetting() executes after
        restoreAllPresets().

        \param setting the Setting object to restore
    */
    void settingRestore(Setting* setting);

    QSettings file_;
    QString prefix_;
    QList<Preset*> presets_;
    QList<Setting*> settings_;
    QList<Setting*> globals_;
    QStringList settingNames_;
    int activePresetIndex_;
    int savedVersion_;
    bool restoring_;
    bool restored_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
