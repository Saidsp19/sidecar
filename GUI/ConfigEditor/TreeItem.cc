#include "TreeItem.h"

using namespace SideCar::GUI::ConfigEditor;

TreeItem::TreeItem(QObject* parent) : Super(parent), parent_(0), name_(""), children_()
{
    ;
}

TreeItem::TreeItem(TreeItem* parent, const QString& name) : Super(parent), parent_(parent), name_(name), children_()
{
    ;
}

TreeItem::~TreeItem()
{
    qDeleteAll(children_);
}

void
TreeItem::clone(TreeItem* item)
{
    name_ = item->name_;
}

bool
TreeItem::insertChildren(int position, const QList<TreeItem*>& items)
{
    if (position < 0 || position > children_.size()) return false;

    foreach (TreeItem* item, items) {
        children_.insert(position++, item);
        item->reparent(this);
    }

    return true;
}

bool
TreeItem::insertChild(int position, TreeItem* item)
{
    if (position < 0 || position > children_.size()) return false;
    children_.insert(position, item);
    item->reparent(this);
    return true;
}

bool
TreeItem::removeChildren(int position, int count)
{
    if (count == 0 || position < 0 || position + count > children_.size()) return false;
    while (count--) delete children_.takeAt(position + count);
    return true;
}

void
TreeItem::setName(const QString& name)
{
    if (name_ != name) {
        name_ = name;
        emitModified();
    }
}

void
TreeItem::emitModified()
{
    emit modified();
}

void
TreeItem::reparent(TreeItem* parent)
{
    if (parent_ != parent) {
        parent_ = parent;
        setParent(parent);
    }
}
