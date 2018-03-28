#ifndef SIDECAR_GUI_CONFIGEDITOR_ROOTITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_ROOTITEM_H

#include "TreeItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class RunnerItem;

class RootItem : public TreeItem {
    Q_OBJECT
    using Super = TreeItem;

public:
    RootItem(QObject* parent);

    Type getType() const { return kRoot; }

    bool canAdopt(Type type) const { return type == kConfiguration; }
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
