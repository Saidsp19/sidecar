#ifndef SIDECAR_GUI_LOGGERTREEITEM_H // -*- C++ -*-
#define SIDECAR_GUI_LOGGERTREEITEM_H

#include "QtCore/QList"
#include "QtCore/QObject"
#include "QtCore/QVariant"

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {

class LoggerTreeItem : public QObject {
    Q_OBJECT
public:
    using Children = QList<LoggerTreeItem*>;
    using ChildrenIterator = QListIterator<LoggerTreeItem*>;

    LoggerTreeItem();

    LoggerTreeItem(Logger::Log& log, LoggerTreeItem* parent);

    ~LoggerTreeItem();

    const QString& getName() const { return name_; }

    const QString& getFullName() const { return fullName_; }

    QVariant getData(int column, int role) const;

    int getIndex() const { return parent_ ? parent_->children_.indexOf(const_cast<LoggerTreeItem*>(this)) : 0; }

    bool canExpand() const { return getNumChildren() > 0; }

    int getInsertionPoint(const QString& name) const;

    void insertChild(int row, LoggerTreeItem* child);

    LoggerTreeItem* getChild(int row) const { return children_.value(row); }

    int getNumChildren() const { return children_.size(); }

    LoggerTreeItem* getParent() const { return parent_; }

    ChildrenIterator getChildrenIterator() const { return children_; }

    Logger::Log& getLog() const { return log_; }

    void dump(std::ostream& os, int indent) const;

private:
    Logger::Log& log_;
    QString name_;
    QString fullName_;
    LoggerTreeItem* parent_;
    Children children_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
