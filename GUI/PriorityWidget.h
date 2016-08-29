#ifndef SIDECAR_GUI_PRIORITYWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_PRIORITYWIDGET_H

#include "QtCore/QModelIndex"
#include "QtCore/QStringList"
#include "QtGui/QItemDelegate"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class LoggerTreeItem;

class PriorityWidget : public QItemDelegate
{
    Q_OBJECT
    using Super = QItemDelegate;
public:

    static Logger::Log& Log();

    PriorityWidget(QObject* parent = 0);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const;

    void setEditorData(QWidget* editor, const QModelIndex& index) const;

    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const;

    void updateEditorGeometry(QWidget* editor,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;

private slots:
    void updatePriority(int priority);

private:
    LoggerTreeItem* getItem(const QModelIndex& index) const;

    QStringList names_;
    mutable QModelIndex active_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
