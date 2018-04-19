#ifndef SIDECAR_GUI_RANGETRUTHEMITTER_ACCELERATIONEDITOR_H // -*- C++ -*-
#define SIDECAR_GUI_RANGETRUTHEMITTER_ACCELERATIONEDITOR_H

#include "QtWidgets/QItemDelegate"

namespace SideCar {
namespace GUI {
namespace RangeTruthEmitter {

class AccelerationEditor : public QItemDelegate {
    Q_OBJECT

public:
    AccelerationEditor(QObject* parent = 0) : QItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    void setEditorData(QWidget* editor, const QModelIndex& index) const;

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

} // end namespace RangeTruthEmitter
} // end namespace GUI
} // end namespace SideCar

#endif
