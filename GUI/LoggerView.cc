#include "QtCore/QSettings"
#include "QtWidgets/QHeaderView"

#include "LogUtils.h"
#include "LoggerTreeItem.h"
#include "LoggerView.h"

using namespace SideCar::GUI;

static const char* const kLoggerViewExpansions = "LoggerViewExpansions";

static const QString&
getTreeItemFullName(const QModelIndex& index)
{
    return static_cast<LoggerTreeItem*>(index.internalPointer())->getFullName();
}

inline std::ostream&
operator<<(std::ostream& os, const QModelIndex& index)
{
    os << "QModelIndex(" << (index.isValid() ? "T " : "F ") << index.row() << "," << index.column();
    if (index.parent().isValid()) os << ' ' << index.parent();
    return os << ")";
}

LoggerView::LoggerView(QWidget* parent) : Super(parent)
{
    setAlternatingRowColors(true);
    connect(this, SIGNAL(expanded(const QModelIndex&)), SLOT(saveExpansionState(const QModelIndex&)));
    connect(this, SIGNAL(collapsed(const QModelIndex&)), SLOT(saveExpansionState(const QModelIndex&)));
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void
LoggerView::rowsInserted(const QModelIndex& parent, int start, int end)
{
    Super::rowsInserted(parent, start, end);
    for (; start <= end; ++start) {
        QModelIndex index(model()->index(start, 0, parent));
        setExpanded(index, getWasExpanded(index));
        index = index.child(0, 0);
        while (index.isValid()) {
            setExpanded(index, getWasExpanded(index));
            index = index.sibling(index.row() + 1, 0);
        }
    }
}

void
LoggerView::saveExpansionState(const QModelIndex& index)
{
    QSettings settings;
    settings.beginGroup(kLoggerViewExpansions);
    settings.setValue(static_cast<LoggerTreeItem*>(index.internalPointer())->getFullName(), isExpanded(index));
    settings.endGroup();
}

bool
LoggerView::getWasExpanded(const QModelIndex& index) const
{
    QSettings settings;
    settings.beginGroup(kLoggerViewExpansions);
    bool value = settings.value(getTreeItemFullName(index), true).toBool();
    settings.endGroup();
    return value;
}
