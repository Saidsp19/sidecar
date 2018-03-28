#ifndef SIDECAR_GUI_CONFIGEDITOR_CONFIGURATIONITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_CONFIGURATIONITEM_H

#include "TreeItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class RootItem;
class RunnerItem;

class ConfigurationItem : public TreeItem {
    Q_OBJECT
    using Super = TreeItem;

public:
    ConfigurationItem(RootItem* parent, const QString& name);

    RootItem* getParent() const;

    Type getType() const { return kConfiguration; }

    bool canAdopt(Type type) const { return type == kRunner; }

    bool canReparent() const { return true; }
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
