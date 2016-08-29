#ifndef SIDECAR_GUI_CONFIGEDITOR_TASKITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_TASKITEM_H

#include "TreeItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class ChannelListItem;
class StreamItem;

class TaskItem : public TreeItem
{
    Q_OBJECT
    using Super = TreeItem;
public:

    enum ChildrenIndices {
	kInputChannels = 0,
	kOutputChannels,
	kNumChildren
    };

    TaskItem(StreamItem* parent, const QString& name);

    StreamItem* getParent() const;

    Type getType() const { return kTask; }

    bool canReparent() const { return true; }

    ChannelListItem* getInputChannels() const;

    ChannelListItem* getOutputChannels() const;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
