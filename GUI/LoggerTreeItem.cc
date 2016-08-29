#include <iostream>

#include "LogUtils.h"
#include "LoggerTreeItem.h"

using namespace SideCar::GUI;

LoggerTreeItem::LoggerTreeItem()
    : log_(Logger::Log::Root()), name_("***"), fullName_("***"),
      parent_(0), children_()
{
    ;
}

LoggerTreeItem::LoggerTreeItem(Logger::Log& log, LoggerTreeItem* parent)
    : log_(log), name_(QString::fromStdString(log.name())),
      fullName_(QString::fromStdString(log.fullName())), parent_(parent),
      children_()
{
    ;
}

LoggerTreeItem::~LoggerTreeItem()
{
    qDeleteAll(children_);
}

QVariant
LoggerTreeItem::getData(int column, int role) const
{
    QVariant value;

    switch (column) {
    case 0:
	if (role == Qt::DisplayRole)
	    value = name_;
	break;

    case 1:
	if (role == Qt::DisplayRole) {
	    QString name = QString::fromStdString(
		Logger::Priority::GetLongName(log_.getPriorityLimit()));
	    value = name[0] + name.mid(1).toLower();
	}
	else if (role == Qt::EditRole) {
	    value = log_.getPriorityLimit();
	}
	break;

    default:
	break;
    }

    return value;
}

int
LoggerTreeItem::getInsertionPoint(const QString& name) const
{
#if 0
    int index = 0;
    for (; index < getNumChildren(); ++index) {
	if (QString::compare(getChild(index)->getName(), name,
                             Qt::CaseInsensitive) >= 0) break;
    }
    return index;
#endif
    return getNumChildren();
}

void
LoggerTreeItem::insertChild(int index, LoggerTreeItem* child)
{
#if 0
    children_.insert(index, child);
#endif
    children_.append(child);
}

void
LoggerTreeItem::dump(std::ostream& os, int indent) const
{
    if (indent) {
	QString spaces(indent++, ' ');
	os << spaces;
    }

    os << "name: " << getFullName() << " obj: " << this
       << " parent: " << parent_ << " children: " << children_.size()
       << std::endl;

    foreach (LoggerTreeItem* child, children_) {
	child->dump(os, indent);
    }
}
