#ifndef SIDECAR_GUI_CONFIGEDITOR_TREEMODEL_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_TREEMODEL_H

#include "QtCore/QAbstractItemModel"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class RootItem;
class TreeItem;

class TreeModel : public QAbstractItemModel {
    Q_OBJECT
public:
    enum Columns { kName = 0, kInfo, kConnections, kNumColumns };

    TreeModel(QObject* parent);

    QVariant data(const QModelIndex& index, int role) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex& index) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    bool insertItems(int position, const QList<TreeItem*>& items, const QModelIndex& parent = QModelIndex());

    bool removeRows(int position, int rows, const QModelIndex& parent = QModelIndex());

private:
    TreeItem* getItem(const QModelIndex& index) const;

    RootItem* root_;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
