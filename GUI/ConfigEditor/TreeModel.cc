#include "TreeModel.h"
#include "RootItem.h"
#include "TreeItem.h"

using namespace SideCar::GUI::ConfigEditor;

TreeModel::TreeModel(QObject* parent) : QAbstractItemModel(parent), root_(new RootItem(this))
{
    ;
}

int
TreeModel::columnCount(const QModelIndex& parent) const
{
    return kNumColumns;
}

QVariant
TreeModel::data(const QModelIndex& index, int role) const
{
    QVariant value;
    if (!index.isValid()) return value;

    if (role != Qt::DisplayRole && role != Qt::EditRole) return value;

    TreeItem* item = getItem(index);
    switch (index.column()) {
    case kName:
        if (role == Qt::DisplayRole || role == Qt::EditRole) value = item->getName();
        break;
    case kInfo: value = item->getInfo(role); break;
    case kConnections: value = item->getConnections(role); break;
    default: break;
    }

    return value;
}

Qt::ItemFlags
TreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags;
    if (index.isValid()) {
        flags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (index.column() == 0) flags |= Qt::ItemIsEditable;
    }
    return flags;
}

TreeItem*
TreeModel::getItem(const QModelIndex& index) const
{
    if (index.isValid()) {
        TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
        if (item) return item;
    }

    return root_;
}

QVariant
TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case kName: return "Name"; break;
        case kInfo: return "Info"; break;
        case kConnections: return "Connections"; break;
        default: break;
        }
    }

    return QVariant();
}

QModelIndex
TreeModel::index(int row, int column, const QModelIndex& parent) const
{
    TreeItem* parentItem = getItem(parent);
    if (!parentItem || row < 0 || row >= parentItem->getChildrenCount() || column < 0 || column >= kNumColumns)
        return QModelIndex();
    return createIndex(row, column, parentItem->getChild(row));
}

QModelIndex
TreeModel::parent(const QModelIndex& index) const
{
    TreeItem* childItem = getItem(index);
    TreeItem* parentItem = childItem->getParent();
    if (!parentItem || parentItem == root_) return QModelIndex();
    return createIndex(parentItem->getMyIndex(), 0, parentItem);
}

bool
TreeModel::insertItems(int position, const QList<TreeItem*>& items, const QModelIndex& parent)
{
    beginInsertRows(parent, position, position + items.size() - 1);
    TreeItem* parentItem = getItem(parent);
    bool ok = parentItem->insertChildren(position, items);
    endInsertRows();
    return ok;
}

bool
TreeModel::removeRows(int position, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, position, position + count - 1);
    TreeItem* parentItem = getItem(parent);
    bool ok = parentItem->removeChildren(position, count);
    endRemoveRows();
    return ok;
}

int
TreeModel::rowCount(const QModelIndex& parent) const
{
    TreeItem* parentItem = getItem(parent);
    return parentItem->getChildrenCount();
}

bool
TreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || index.column() != kName) return false;
    TreeItem* item = getItem(index);
    item->setName(value.toString());
    return true;
}
