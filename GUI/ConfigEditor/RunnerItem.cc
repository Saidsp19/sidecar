#include "ConfigurationItem.h"
#include "RunnerItem.h"
#include "StreamItem.h"

using namespace SideCar::GUI::ConfigEditor;

RunnerItem::RunnerItem(ConfigurationItem* parent, const QString& name,
                       const QString& hostName,
                       const QString& multicastAddress)
    : Super(parent, name), hostName_(hostName),
      multicastAddress_(multicastAddress)
{
    ;
}

ConfigurationItem*
RunnerItem::getParent() const
{
    return dynamic_cast<ConfigurationItem*>(Super::getParent());
}

void
RunnerItem::setHostName(const QString& hostName)
{
    if (hostName_ != hostName) {
	hostName_ = hostName;
	emitModified();
    }
}

void
RunnerItem::setMulticastAddress(const QString& multicastAddress)
{
    if (multicastAddress_ != multicastAddress) {
	multicastAddress_ = multicastAddress;
	emitModified();
    }
}
