#include "QtGui/QDoubleSpinBox"

#include "AccelerationEditor.h"

using namespace SideCar::GUI::RangeTruthEmitter;

QWidget*
AccelerationEditor::createEditor(QWidget* parent,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
    editor->setRange(-999.0, 999.0);
    editor->setSingleStep(10);
    // editor->setSuffix(" m/s2");
    editor->setDecimals(2);
    return editor;
}

void
AccelerationEditor::setEditorData(QWidget* editor, const QModelIndex& index)
    const
{
    double value = index.model()->data(index, Qt::EditRole).toDouble();
    QDoubleSpinBox* w = static_cast<QDoubleSpinBox*>(editor);
    w->setValue(value);
    w->selectAll();
}

void
AccelerationEditor::setModelData(QWidget* editor, QAbstractItemModel* model,
                                 const QModelIndex& index) const
{
    QDoubleSpinBox* e = static_cast<QDoubleSpinBox*>(editor);
    e->interpretText();
    double value = e->value();
    model->setData(index, value, Qt::EditRole);
}

void
AccelerationEditor::updateEditorGeometry(QWidget* editor,
                                         const QStyleOptionViewItem& option,
                                         const QModelIndex& index) const
{
    editor->setGeometry(option.rect);
}
