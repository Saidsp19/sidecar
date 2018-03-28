#ifndef SIDECAR_GUI_CONFIGEDITOR_STREAMITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_STREAMITEM_H

#include "TreeItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class RunnerItem;
class TaskItem;

class StreamItem : public TreeItem {
    Q_OBJECT
    using Super = TreeItem;

public:
    StreamItem(RunnerItem* parent, const QString& name);

    RunnerItem* getParent() const;

    Type getType() const { return kStream; }

    bool canAdopt(Type type) const { return type == kTask || type == kAlgorithm; }

    bool canReparent() const { return true; }
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
