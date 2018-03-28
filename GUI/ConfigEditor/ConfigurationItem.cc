#include "ConfigurationItem.h"
#include "RootItem.h"
#include "RunnerItem.h"

using namespace SideCar::GUI::ConfigEditor;

ConfigurationItem::ConfigurationItem(RootItem* parent, const QString& name) : Super(parent, name)
{
    ;
}

RootItem*
ConfigurationItem::getParent() const
{
    return dynamic_cast<RootItem*>(Super::getParent());
}
