#include "QtCore/QSettings"

#include "AppBase.h"
#include "LogUtils.h"
#include "Preset.h"
#include "PresetManager.h"
#include "Setting.h"

static const char* const kVersion = "Version";    
static const char* const kGlobals = "Globals";
static const char* const kLastActive = "LastActive";
static const char* const kName = "Name";
static const char* const kPresetData = "PresetData";
static const char* const kPresetNames = "PresetNames";
static const char* const kValue = "Value";
static const char* const kDefault = "Default";

using namespace SideCar::GUI;

Logger::Log&
PresetManager::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.PresetManager");
    return log_;
}

PresetManager::PresetManager(const QString& prefix, QObject* parent)
    : QObject(parent), file_(), prefix_(prefix), presets_(),
      settings_(), globals_(), settingNames_(), activePresetIndex_(0),
      restoring_(false), restored_(false)
{
    savedVersion_ = file_.value(kVersion, 1).toInt();

    // Create an unnamed Preset object to represent Setting values when no Preset is active. It is always the
    // first entry in the presets_ list.
    //
    presets_.append(new Preset(kDefault, this));
}

QStringList
PresetManager::getPresetNames() const
{
    QStringList names;
    for (int index = 0; index < presets_.size(); ++index) {
	names.append(presets_[index]->getName());
    }

    return names;
}

const QString&
PresetManager::getPresetName(int index) const
{
    return presets_[index]->getName();
}

Preset*
PresetManager::getPreset(const QString& name) const
{
    for (int index = 0; index < presets_.size(); ++index) {
	if (presets_[index]->getName() == name)
	    return presets_[index];
    }

    return 0;
}

void
PresetManager::restoreAllPresets()
{
    Logger::ProcLog log("restoreAllPresets", Log());
    LOGINFO << std::endl;

    restoring_ = true;
    file_.beginGroup(prefix_);
    {
	// Load in the global settings.
	//
	file_.beginGroup(kGlobals);
	{
	    for (int index = 0; index < globals_.size(); ++index) {
		Setting* setting = globals_[index];
		QVariant value = file_.value(setting->getName(),
                                             setting->getOpaqueValue());
		if (isOldVersion())
		    value = upgradeSetting(setting->getName(), value);
		setting->restoreValue(value);
	    }
	}
	file_.endGroup();

	// Remove any existing named presets (but not the first entry)
	//
	while (presets_.size())
	    delete presets_.takeLast();

	// Load in the names of defined presets. This will create a new Preset object for each name that was
	// last saved.
	//
	int size = file_.beginReadArray(kPresetNames);
	for (int index = 0; index < size; ++index) {
	    file_.setArrayIndex(index);
	    QString name = file_.value(kName).toString();
	    if (! name.isEmpty()) 
		presets_.append(new Preset(name, this));
	}
	file_.endArray();

	// Temporary hack for old config file versions - if the first entry is NOT the 'default' one, create one
	// and place at the beginning of the list.
	//
	if (presets_.empty() || presets_[0]->getName() != kDefault) {
	    presets_.insert(0, new Preset(kDefault, this));
	}

	emit presetNamesChanged(getPresetNames());

	// Restore the index of the last preset.
	//
	activePresetIndex_ = file_.value(kLastActive, 0).toInt();

	// Load in the preset setting values. If restoring the active preset, have it also apply the value to
	// the Setting object.
	//
	file_.beginGroup(kPresetData);
	{
	    for (int index = 0; index < presets_.size(); ++index) {
		presets_[index]->restore(file_, settings_,
                                         index == activePresetIndex_);
		if (index)
		    emit presetDirtyStateChanged(index, false);
	    }
	}
	file_.endGroup();		// kPresetData

	emit activePresetChanged(activePresetIndex_);
    }

    file_.endGroup();		// prefix_
    restoring_ = false;
    restored_ = true;
}

void
PresetManager::saveAllPresets(bool saveChanges)
{
    file_.setValue(kVersion, getCurrentVersion());
    file_.beginGroup(prefix_);
    {

	// Save the global settings.
	//
	file_.beginGroup(kGlobals);
	{
	    for (int index = 0; index < globals_.size(); ++index) {
		Setting* setting = globals_[index];
		file_.setValue(setting->getName(),
                               setting->getOpaqueValue());
	    }
	}
	file_.endGroup();

	// Save the names of the defined presets.
	//
	file_.beginWriteArray(kPresetNames, presets_.size());
	for (int index = 0; index < presets_.size(); ++index) {
	    file_.setArrayIndex(index);
	    file_.setValue(kName, presets_[index]->getName());
	}
	file_.endArray();

	// Save the preset setting values.
	//
	file_.beginGroup(kPresetData);
	{
	    for (int index = 0; index < presets_.size(); ++index) {
		bool dirty = getPresetIsDirty(index);
		if (! dirty || (dirty && saveChanges)) {
		    presets_[index]->save(file_, settingNames_);
		    if (index)
			emit presetDirtyStateChanged(index, false);
		}
	    }
	}
	file_.endGroup();	// kPresetData

	// Save the index of the active preset.
	//
	file_.setValue(kLastActive, activePresetIndex_);
    }
    file_.endGroup();		// prefix_

    savedVersion_ = getCurrentVersion();
}

void
PresetManager::restorePreset(int index)
{
    restoring_ = true;
    file_.beginGroup(prefix_);
    {
	file_.beginGroup(kPresetData);
	{
	    presets_[index]->restore(file_, settings_, true);
	}
	file_.endGroup();	// kPresetData
    }
    file_.endGroup();		// prefix_
    restoring_ = false;

    if (index)
	emit presetDirtyStateChanged(index, false);
}

void
PresetManager::savePreset(int index)
{
    file_.beginGroup(prefix_);
    {
	file_.beginGroup(kPresetData);
	{
	    presets_[index]->save(file_, settingNames_);
	}
	file_.endGroup();	// kPresetData
    }
    file_.endGroup();		// prefix_

    if (index) 
	emit presetDirtyStateChanged(index, false);
}

size_t
PresetManager::addSetting(Setting* setting, bool global)
{
    size_t index;
    if (global) {
	index = globals_.size();
	globals_.append(setting);
	if (restored_)
	    globalRestore(setting);
	connect(setting, SIGNAL(save(int, const QVariant&)),
                SLOT(globalSave(int, const QVariant&)));
    }
    else {
	index = settings_.size();
	settings_.append(setting);
	settingNames_.append(setting->getName());
	connect(setting, SIGNAL(save(int, const QVariant&)),
                SLOT(settingSave(int, const QVariant&)));
	if (restored_)
	    settingRestore(setting);
    }
    return index;
}

void
PresetManager::globalRestore(Setting* setting)
{
    restoring_ = true;
    file_.beginGroup(prefix_);
    {
	file_.beginGroup(kGlobals);
	{
	    setting->setOpaqueValue(file_.value(
                                        setting->getName(),
                                        setting->getOpaqueValue()));
	}
	file_.endGroup();
    }
    file_.endGroup();
    restoring_ = false;
}

void
PresetManager::settingRestore(Setting* setting)
{
    restoring_ = true;
    file_.beginGroup(prefix_);
    {
	file_.beginGroup(kPresetData);
	{
	    for (int index = 0; index < presets_.size(); ++index) {
		presets_[index]->restoreOne(file_, setting);
	    }
	}
	file_.endGroup();
    }
    file_.endGroup();
    restoring_ = false;
}

Setting*
PresetManager::getSetting(const QString& name) const
{
    for (int index = 0; index < settingNames_.size(); ++index) {
	if (settingNames_[index] == name)
	    return settings_[index];
    }

    return 0;
}

Setting*
PresetManager::getGlobal(const QString& name) const
{
    for (int index = 0; index < globals_.size(); ++index) {
	if (globals_[index]->getName() == name)
	    return globals_[index];
    }

    return 0;
}

void
PresetManager::settingSave(int index, const QVariant& value)
{
    static Logger::ProcLog log("settingSave", Log());
    LOGINFO << "index: " << index << " value: " << value.toString()
	    << std::endl;

    if (index == -1 || restoring_) return;

    LOGDEBUG << "name: " << settings_[index]->getName() << std::endl;

    // Detect when a preset becomes dirty due to a setting change. Emit a signal when this occurs, but only if
    // the setting is not the 'default' preset.
    //
    bool wasDirty = getActiveIsDirty();
    presets_[activePresetIndex_]->setValue(index, value);
    if (! wasDirty && getActiveIsDirty() && activePresetIndex_ > 0)
	emit presetDirtyStateChanged(activePresetIndex_, true);

    // If this is the 'default' setting, we always write out the setting change.
    //
    if (activePresetIndex_ == 0) {
	file_.beginGroup(prefix_);
	{
	    file_.beginGroup(kPresetData);
	    {
		file_.beginGroup(presets_[activePresetIndex_]->getName());
		{
		    file_.setValue(settings_[index]->getName(), value);
		}
		file_.endGroup();
	    }
	    file_.endGroup();
	}
	file_.endGroup();
    }
}

void
PresetManager::globalSave(int index, const QVariant& value)
{
    if (index != -1) {
	file_.beginGroup(prefix_);
	{
	    file_.beginGroup(kGlobals);
	    {
		file_.setValue(globals_[index]->getName(), value);
	    }
	    file_.endGroup();
	}
	file_.endGroup();
    }
}

bool
PresetManager::addPreset(const QString& name)
{
    // Make sure that the new name is unique.
    //
    for (int index = 0; index < presets_.size(); ++index) {
	if (presets_[index]->getName() == name)
	    return false;
    }

    // Create a new Preset object that is a copy of the active one.
    //
    Preset* preset = presets_[activePresetIndex_]->duplicate(name);
    presets_.append(preset);

    file_.beginGroup(prefix_);

    // Write the array of preset names to the configuration file.
    //
    file_.beginWriteArray(kPresetNames, presets_.size());
    for (int nameIndex = 0; nameIndex < presets_.size(); ++nameIndex) {
	file_.setArrayIndex(nameIndex);
	file_.setValue(kName, presets_[nameIndex]->getName());
    }
    file_.endArray();

    activePresetIndex_ = presets_.size() - 1;

    // Save the new preset to the configuration file
    //
    file_.beginGroup(kPresetData);
    presets_[activePresetIndex_]->save(file_, settingNames_);
    file_.endGroup();		// kPresetData

    file_.setValue(kLastActive, activePresetIndex_);

    file_.endGroup();		// prefix_

    emit presetNamesChanged(getPresetNames());
    emit activePresetChanged(activePresetIndex_);

    return true;
}

bool
PresetManager::removePreset(int index)
{
    // Don't allow deleting the 'default' preset.
    //
    if (index < 1 || index > presets_.size() - 1)
	return false;

    delete presets_.takeAt(index);

    file_.beginGroup(prefix_);

    // Write the array of preset names to the configuration file.
    //
    file_.beginWriteArray(kPresetNames, presets_.size());
    for (int nameIndex = 0; nameIndex < presets_.size(); ++nameIndex) {
	file_.setArrayIndex(nameIndex);
	file_.setValue(kName, presets_[nameIndex]->getName());
    }
    file_.endArray();

    // Are we removing the active preset?
    //
    if (activePresetIndex_ != index) {

	// No. If removing an entry prior to the active one, just adjust the active index value and record it,
	// but do not announce any change.
	//
	if (index < activePresetIndex_) {
	    --activePresetIndex_;
	    file_.setValue(kLastActive, activePresetIndex_);
	}
    }
    else {

	// Yes. Make the 'default' preset the active one.
	//
	activePresetIndex_ = 0;
	presets_[activePresetIndex_]->apply(settings_);
	file_.setValue(kLastActive, activePresetIndex_);
	emit activePresetChanged(activePresetIndex_);
    }

    file_.endGroup();		// prefix_

    emit presetNamesChanged(getPresetNames());

    return true;
}

bool
PresetManager::applyPreset(int index)
{
    if (index < 0 || index > presets_.size() - 1)
	return false;

    if (activePresetIndex_ == index)
	return true;

    restoring_ = true;
    
    activePresetIndex_ = index;
    presets_[activePresetIndex_]->apply(settings_);

    // Remember the new active index.
    //
    file_.beginGroup(prefix_);
    file_.setValue(kLastActive, activePresetIndex_);
    file_.endGroup();

    restoring_ = false;

    emit activePresetChanged(index);

    return true;
}

bool
PresetManager::getPresetIsDirty(int index) const
{
    // The 'default' preset is never dirty.
    //
    return index > 0 && index < presets_.size() &&
                                presets_[index]->isDirty();
}

bool
PresetManager::getAnyIsDirty() const
{
    // Scan all presets but the first, 'default' one to see if any are dirty.
    //
    for (int index = 1; index < presets_.size(); ++index)
	if (presets_[index]->isDirty())
	    return true;
    return false;
}
