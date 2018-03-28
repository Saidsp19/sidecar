#ifndef SIDECAR_GUI_CONFIGEDITOR_TREEITEM_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_TREEITEM_H

#include "QtCore/QList"
#include "QtCore/QObject"
#include "QtCore/QVariant"

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

class TreeItem;

using TreeItemList = QList<TreeItem*>;

class TreeItem : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    enum Type {
        kRoot = 0,
        kConfiguration,
        kRunner,
        kStream,
        kTask,
        kChannelList,
        kChannel,
        kConnection,
        kAlgorithm,
        kParameterList,
        kParameter,
        kNumTypes
    };

    virtual ~TreeItem();

    virtual void clone(TreeItem* item);

    virtual Type getType() const = 0;

    virtual bool canAdopt(Type type) const { return false; }

    virtual bool canReparent() const { return false; }

    const QString& getName() const { return name_; }

    virtual QVariant getInfo(int role) const { return QVariant(); }

    virtual QVariant getConnections(int role) const { return QVariant(); }

    int getIndexOf(const TreeItem* child) const { return children_.indexOf(const_cast<TreeItem*>(child)); }

    int getMyIndex() const { return parent_ ? parent_->getIndexOf(this) : 0; }

    TreeItem* getParent() const { return parent_; }

    TreeItem* getChild(int index) const { return children_[index]; }

    int getChildrenCount() const { return children_.count(); }

    bool insertChildren(int position, const TreeItemList& items);

    bool insertChild(int position, TreeItem* item);

    bool removeChildren(int position, int count);

public slots:

    void setName(const QString& name);

signals:

    void modified();

protected:
    TreeItem(QObject* parent);

    TreeItem(TreeItem* parent, const QString& name);

    void emitModified();

    virtual void reparent(TreeItem* parent);

private:
    TreeItem* parent_;
    QString name_;
    TreeItemList children_;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

#endif
