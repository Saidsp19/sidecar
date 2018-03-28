#include "QtCore/QAbstractItemModel"
#include "QtCore/QStringList"
#include "QtGui/QComboBox"

#include "LogUtils.h"
#include "LoggerTreeItem.h"
#include "PriorityWidget.h"

using namespace SideCar::GUI;

Logger::Log&
PriorityWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.PriorityWidget");
    return log_;
}

PriorityWidget::PriorityWidget(QObject* parent) : Super(parent), names_()
{
    for (int index = Logger::Priority::kNone; index < Logger::Priority::kNumLevels; ++index) {
        QString name = QString::fromStdString(Logger::Priority::GetLongName(Logger::Priority::Level(index)));
        names_ << (name[0] + name.mid(1).toLower());
    }
}

QWidget*
PriorityWidget::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QWidget* widget;
    if (index.column() == 1) {
        active_ = index;
        QComboBox* editor = new QComboBox(parent);
        widget = editor;
        editor->addItems(names_);
        editor->installEventFilter(const_cast<PriorityWidget*>(this));
        connect(editor, SIGNAL(currentIndexChanged(int)), SLOT(updatePriority(int)));
    } else {
        widget = Super::createEditor(parent, option, index);
    }

    return widget;
}

void
PriorityWidget::updatePriority(int priority)
{
    const_cast<QAbstractItemModel*>(active_.model())->setData(active_, priority, Qt::EditRole);
}

void
PriorityWidget::setEditorData(QWidget* widget, const QModelIndex& index) const
{
    LoggerTreeItem* node = getItem(index);
    if (!node) return;
    QComboBox* editor = static_cast<QComboBox*>(widget);
    editor->setCurrentIndex(node->getLog().getPriorityLimit());
}

void
PriorityWidget::setModelData(QWidget* widget, QAbstractItemModel* model, const QModelIndex& index) const
{
    LoggerTreeItem* node = getItem(index);
    if (!node) return;

    QComboBox* editor = static_cast<QComboBox*>(widget);
    int value = editor->currentIndex();
    model->setData(index, value, Qt::EditRole);
    model->setData(index, QString::fromStdString(Logger::Priority::GetLongName(Logger::Priority::Level(value))),
                   Qt::DisplayRole);
}

void
PriorityWidget::updateEditorGeometry(QWidget* widget, const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const
{
    widget->setGeometry(option.rect);
}

LoggerTreeItem*
PriorityWidget::getItem(const QModelIndex& index) const
{
    if (!index.isValid()) return 0;
    return static_cast<LoggerTreeItem*>(index.internalPointer());
}

QSize
PriorityWidget::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize size = Super::sizeHint(option, index);
    size.rheight() += 10;
    return size;
}
