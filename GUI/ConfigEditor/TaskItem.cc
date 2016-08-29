#include "ChannelListItem.h"
#include "StreamItem.h"
#include "TaskItem.h"

using namespace SideCar::GUI::ConfigEditor;

TaskItem::TaskItem(StreamItem* parent, const QString& name)
    : Super(parent, name)
{
    insertChild(kInputChannels, new ChannelListItem(this, "Inputs"));
    insertChild(kOutputChannels, new ChannelListItem(this, "Outputs"));
}

StreamItem*
TaskItem::getParent() const
{
    return dynamic_cast<StreamItem*>(Super::getParent());
}

ChannelListItem*
TaskItem::getInputChannels() const
{
    return dynamic_cast<ChannelListItem*>(getChild(kInputChannels));
}

ChannelListItem*
TaskItem::getOutputChannels() const
{
    return dynamic_cast<ChannelListItem*>(getChild(kOutputChannels));
}
