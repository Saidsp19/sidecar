#include "ParameterListItem.h"
#include "AlgorithmItem.h"
#include "ParameterItem.h"

using namespace SideCar::GUI::ConfigEditor;

ParameterListItem::ParameterListItem(AlgorithmItem* parent) : Super(parent, "Parameters")
{
    ;
}

AlgorithmItem*
ParameterListItem::getParent() const
{
    return dynamic_cast<AlgorithmItem*>(Super::getParent());
}

ParameterItem*
ParameterListItem::getChild(int index) const
{
    return dynamic_cast<ParameterItem*>(Super::getChild(index));
}
