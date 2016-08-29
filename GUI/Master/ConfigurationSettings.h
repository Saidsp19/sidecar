#ifndef SIDECAR_GUI_MASTER_CONFIGURATIONSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_CONFIGURATIONSETTINGS_H

#include "GUI/StringListSetting.h"
#include "GUI/SettingsBlock.h"

namespace SideCar {
namespace GUI {
namespace Master {

class ConfigurationSettings : public SettingsBlock
{
    Q_OBJECT
    using Super = SettingsBlock;
public:

    ConfigurationSettings(StringListSetting* configPaths);

    const QStringList& getConfigPaths() const
	{ return configPaths_->getValue(); }

    void setConfigPaths(const QStringList& paths)
	{ configPaths_->setValue(paths); }

private:
    StringListSetting* configPaths_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
