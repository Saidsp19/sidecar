#ifndef SIDECAR_GUI_CONFIGEDITOR_CHANNELLISTITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_CHANNELLISTITEM_H

#include "TreeItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class ChannelItem;
class TaskItem;

class ChannelListItem : public TreeItem {
    Q_OBJECT
    using Super = TreeItem;

public:
    ChannelListItem(TaskItem* parent, const QString& name);

    Type getType() const { return kChannelList; }

    bool canAdopt(Type type) const { return type == kChannel; }

    TaskItem* getParent() const;

    ChannelItem* getChild(int index) const;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
