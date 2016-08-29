#include "ChannelItem.h"
#include "ChannelListItem.h"
#include "TaskItem.h"

using namespace SideCar::GUI::ConfigEditor;

ChannelListItem::ChannelListItem(TaskItem* parent, const QString& name)
    : Super(parent, name)
{
    ;
}

TaskItem*
ChannelListItem::getParent() const
{
    return dynamic_cast<TaskItem*>(Super::getParent());
}

ChannelItem*
ChannelListItem::getChild(int index) const
{
    return dynamic_cast<ChannelItem*>(Super::getChild(index));
}
