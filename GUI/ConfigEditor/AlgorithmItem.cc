#include "AlgorithmItem.h"
#include "ParameterListItem.h"

using namespace SideCar::GUI::ConfigEditor;

AlgorithmItem::AlgorithmItem(StreamItem* parent, const QString& name, const QString& dll) : Super(parent, name)
{
    insertChild(kParameters, new ParameterListItem(this));
}

ParameterListItem*
AlgorithmItem::getParameters() const
{
    return dynamic_cast<ParameterListItem*>(getChild(kParameters));
}
