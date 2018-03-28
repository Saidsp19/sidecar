#include "QtGui/QDoubleSpinBox"

#include "DurationEditor.h"

using namespace SideCar::GUI::RangeTruthEmitter;

QWidget*
DurationEditor::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
    editor->setRange(0.01, 10000.0);
    editor->setSingleStep(5);
    editor->setSuffix(" sec");
    editor->setDecimals(2);
    return editor;
}

void
DurationEditor::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    double value = index.model()->data(index, Qt::EditRole).toDouble();
    QDoubleSpinBox* w = static_cast<QDoubleSpinBox*>(editor);
    w->setValue(value);
    w->selectAll();
}

void
DurationEditor::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QDoubleSpinBox* e = static_cast<QDoubleSpinBox*>(editor);
    e->interpretText();
    double value = e->value();
    model->setData(index, value, Qt::EditRole);
}

void
DurationEditor::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const
{
    editor->setGeometry(option.rect);
}
