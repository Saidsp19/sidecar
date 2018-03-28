#ifndef SIDECAR_GUI_CONFIGEDITOR_PARAMETERITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_PARAMETERITEM_H

#include "TreeItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class ParameterListItem;

class ParameterItem : public TreeItem {
    Q_OBJECT
    using Super = TreeItem;

public:
    ParameterItem(ParameterListItem* parent, const QString& name);

    Type getType() const { return kParameter; }
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
