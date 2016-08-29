#include "QtGui/QDoubleSpinBox"

#include "VelocityEditor.h"

using namespace SideCar::GUI::RangeTruthEmitter;

QWidget*
VelocityEditor::createEditor(QWidget* parent,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
    editor->setRange(-99999.0, 99999.0);
    editor->setSingleStep(100);
    // editor->setSuffix(" m/s");
    editor->setDecimals(2);
    editor->setValue(index.model()->data(index).toDouble());
    return editor;
}

void
VelocityEditor::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    double value = index.model()->data(index).toDouble();
    QDoubleSpinBox* w = static_cast<QDoubleSpinBox*>(editor);
    w->setValue(value);
    w->selectAll();
}

void
VelocityEditor::setModelData(QWidget* editor, QAbstractItemModel* model,
                             const QModelIndex& index) const
{
    QDoubleSpinBox* e = static_cast<QDoubleSpinBox*>(editor);
    e->interpretText();
    double value = e->value();
    model->setData(index, value);
}

void
VelocityEditor::updateEditorGeometry(QWidget* editor,
                                     const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const
{
    editor->setGeometry(option.rect);
}
