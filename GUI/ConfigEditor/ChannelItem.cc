#include "QtAlgorithms"

#include "ChannelListItem.h"
#include "ChannelItem.h"
#include "ConnectionItem.h"
#include "MessageType.h"
#include "TaskItem.h"

using namespace SideCar::GUI::ConfigEditor;

ChannelItem::ChannelItem(ChannelListItem* parent, const QString& name,
                         const QString& mappedName,
                         const MessageType* messageType)
    : TreeItem(parent, name), mappedName_(mappedName),
      messageType_(messageType)
{
    connectionName_ = QString("%1/%2")
	.arg(parent->getParent()->getName())
	.arg(parent->getChildrenCount());
}

ChannelListItem*
ChannelItem::getParent() const
{
    return dynamic_cast<ChannelListItem*>(Super::getParent());
}

ConnectionItem*
ChannelItem::getChild(int index) const
{
    return dynamic_cast<ConnectionItem*>(Super::getChild(index));
}

void
ChannelItem::setMappedName(const QString& mappedName)
{
    if (mappedName_ != mappedName) {
	mappedName_ = mappedName;
	emitModified();
    }
}

void
ChannelItem::setMessageType(const MessageType* messageType)
{
    if (messageType_ != messageType) {
	messageType_ = messageType;
	emitModified();
    }
}
