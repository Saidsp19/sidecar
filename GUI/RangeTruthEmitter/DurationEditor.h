#ifndef SIDECAR_GUI_RANGETRUTHEMITTER_DURATIONEDITOR_H // -*- C++ -*-
#define SIDECAR_GUI_RANGETRUTHEMITTER_DURATIONEDITOR_H

#include "QtGui/QItemDelegate"

namespace SideCar {
namespace GUI {
namespace RangeTruthEmitter {

class DurationEditor : public QItemDelegate {
    Q_OBJECT

public:
    DurationEditor(QObject* parent = 0) : QItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    void setEditorData(QWidget* editor, const QModelIndex& index) const;

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

} // end namespace RangeTruthEmitter
} // end namespace GUI
} // end namespace SideCar

#endif
