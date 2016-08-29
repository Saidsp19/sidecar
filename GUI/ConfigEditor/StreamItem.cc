#include "RunnerItem.h"
#include "StreamItem.h"
#include "TaskItem.h"

using namespace SideCar::GUI::ConfigEditor;

StreamItem::StreamItem(RunnerItem* parent, const QString& name)
    : Super(parent, name)
{
    ;
}

RunnerItem*
StreamItem::getParent() const
{
    return dynamic_cast<RunnerItem*>(Super::getParent());
}
