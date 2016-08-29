#include "ChannelItem.h"
#include "ConnectionItem.h"

using namespace SideCar::GUI::ConfigEditor;

ConnectionItem::ConnectionItem(ChannelItem* parent, ChannelItem* to)
    : Super(parent, to->getConnectionName()), to_(to)
{
    ;
}

ChannelItem*
ConnectionItem::getParent() const
{
    return dynamic_cast<ChannelItem*>(Super::getParent());
}
