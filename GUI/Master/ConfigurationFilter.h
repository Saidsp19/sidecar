#ifndef SIDECAR_GUI_MASTER_CONFIGURATIONFILTER_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_CONFIGURATIONFILTER_H

#include "QtCore/QMetaType"
#include "QtCore/QString"
#include "QtCore/QStringList"

namespace SideCar {
namespace GUI {
namespace Master {

/** Configuration filter setting. Limits the effects of certain Master actions to the configuration name held by
    the filter.
*/
class ConfigurationFilter
{
public:

    ConfigurationFilter() : loaded_(), filter_() {}

    ConfigurationFilter(const QStringList& loaded)
	: loaded_(loaded), filter_() {}

    ConfigurationFilter(const QStringList& loaded, const QString& value)
	: loaded_(loaded), filter_(value) {}

    bool passes(const QString& name) const
	{ return filter_.contains(name) || ! loaded_.contains(name); }

    const QString& getDisplayText() const;

    const QString& getValue() const { return value_; }

    bool operator==(const ConfigurationFilter& rhs) const
	{ return value_ == rhs.value_; }
    
    bool operator!=(const ConfigurationFilter& rhs) const
	{ return value_ != rhs.value_; }

private:
    QStringList loaded_;
    QStringList filter_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

Q_DECLARE_METATYPE(SideCar::GUI::Master::ConfigurationFilter);

/** \file
 */

#endif
