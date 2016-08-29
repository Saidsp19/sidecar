#ifndef SIDECAR_GUI_CONFIGEDITOR_PARAMETERLISTITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_PARAMETERLISTITEM_H

#include "TreeItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class AlgorithmItem;
class ParameterItem;

class ParameterListItem : public TreeItem
{
    Q_OBJECT
    using Super = TreeItem;
public:

    ParameterListItem(AlgorithmItem* parent);

    AlgorithmItem* getParent() const;

    Type getType() const { return kParameterList; }

    bool canAdopt(Type type) const { return type == kParameter; }

    ParameterItem* getChild(int index) const;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
