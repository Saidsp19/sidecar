#ifndef SIDECAR_GUI_CONFIGEDITOR_ALGORITHMITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_ALGORITHMITEM_H

#include "TaskItem.h"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class ParameterListItem;

class AlgorithmItem : public TaskItem
{
    Q_OBJECT
    using Super = TaskItem;
public:

    enum ChildrenIndices {
	kParameters = Super::kNumChildren,
	kNumChildren
    };

    AlgorithmItem(StreamItem* parent, const QString& name,
                  const QString& dllName);

    Type getType() const { return kAlgorithm; }

    ParameterListItem* getParameters() const;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
