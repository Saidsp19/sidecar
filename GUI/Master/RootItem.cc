#include "RootItem.h"
#include "ConfigurationItem.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

ConfigurationItem*
RootItem::find(const QString& name) const
{
    for (int index = 0; index < getNumChildren(); ++index) {
        ConfigurationItem* item = getChild(index);
        if (item->getName() == name) return item;
    }
    return 0;
}

ConfigurationItem*
RootItem::getChild(int index) const
{
    return static_cast<ConfigurationItem*>(Super::getChild(index));
}
