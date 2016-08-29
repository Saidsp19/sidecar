#include <algorithm>
#include <iostream>

#include "QtCore/QModelIndex"
#include "QtCore/QSettings"
#include "QtCore/QVariant"

#include "LoggerModel.h"
#include "LoggerTreeItem.h"
#include "LogUtils.h"

using namespace SideCar::GUI;

static const char* const kLoggerLogLevels = "LoggerLogLevels";

inline std::ostream&
operator<<(std::ostream& os, const QModelIndex& index)
{
    os << "QModelIndex(" << (index.isValid() ? "T " : "F ") << index.row() << "," << index.column() << " "
       << index.internalPointer();
    if (index.parent().isValid()) os << ' ' << index.parent();
    return os << ")";
}

Logger::Log&
LoggerModel::Log()
{
    static auto& log_ = Logger::Log::Find("SideCar.GUI.LoggerModel");
    return log_;
}

LoggerModel::LoggerModel(QObject* parent)
    : QAbstractItemModel(parent), hash_(), root_(new LoggerTreeItem)
{
    connect(this, SIGNAL(recordNewDevice(const QString&)), this, SLOT(doRecordNewDevice(const QString&)),
            Qt::QueuedConnection);
    // dump();
}

LoggerModel::~LoggerModel()
{
    delete root_;
}

void
LoggerModel::initialize()
{
    auto names = Logger::Log::GetNames([this](auto& d){newDevice(d);});
    for (auto s: names) addDevice(s);
}

void
LoggerModel::newDevice(Logger::Log& device)
{
    emit recordNewDevice(QString::fromStdString(device.fullName()));
}

void
LoggerModel::doRecordNewDevice(const QString& fullName)
{
    Logger::ProcLog log("doRecordNewDevice", Log());
    LOGDEBUG2 << fullName << std::endl;
    auto& device = Logger::Log::Find(fullName.toStdString());
    auto pos = hash_.find(QString::fromStdString(device.getParent()->fullName()));
    if (pos != hash_.end()) {
	makeTreeItem(device, pos.value());
    }
}

LoggerTreeItem*
LoggerModel::addDevice(const std::string& fullName)
{
    Logger::ProcLog log("addDevice", Log());
    LOGINFO << fullName << std::endl;

    auto& device = Logger::Log::Find(fullName);
    auto qname = QString::fromStdString(fullName);

    auto pos = hash_.find(qname);
    if (pos != hash_.end()) {
	return pos.value();
    }

    LoggerTreeItem* parentItem = nullptr;
    auto parent = device.getParent();
    if (parent) {
	parentItem = addDevice(parent->fullName());
    }
    else {
	parentItem = root_;
    }

    return makeTreeItem(device, parentItem);
}

LoggerTreeItem*
LoggerModel::makeTreeItem(Logger::Log& device, LoggerTreeItem* parent)
{
    Logger::ProcLog log("makeTreeItem", Log());
    LOGINFO << device.fullName() << ' ' << parent << std::endl;

    auto node = new LoggerTreeItem(device, parent);
    hash_.insert(node->getFullName(), node);

    QModelIndex parentIndex;
    auto row = 0;
    if (parent != root_) {
	parentIndex = createIndex(parent->getIndex(), 0, parent);
	row = parent->getInsertionPoint(node->getName());
    }

    QSettings settings;
    settings.beginGroup(kLoggerLogLevels);
    auto priority = settings.value(node->getFullName(), device.getPriorityLimit()).toInt();
    LOGDEBUG3 << node->getFullName() << ' ' << priority << std::endl;

    if (priority != device.getPriorityLimit()) {
	device.setPriorityLimit(Logger::Priority::Level(priority));
    }

    LOGDEBUG3 << "parentIndex: " << parentIndex << " row: " << row << std::endl;
    beginInsertRows(parentIndex, row, row);
    parent->insertChild(row, node);
    // dump();
    endInsertRows();

    return node;
}

int
LoggerModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0) {
	return 0;
    }
    auto parentNode = getTreeItem(parent);
    return parentNode->getNumChildren();
}

int
LoggerModel::columnCount(const QModelIndex& parent) const
{
    return 2;
}

QVariant
LoggerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant value;
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
	value = QString(section ? "Priority" : "Name");
    }
    return value;
}

QVariant
LoggerModel::data(const QModelIndex& index, int role) const
{
    QVariant value;
    if (index.isValid()) {
	auto node = getTreeItem(index);
	if (node) {
	    value = node->getData(index.column(), role);
        }
    }
    return value;
}

Qt::ItemFlags
LoggerModel::flags(const QModelIndex& index) const
{
    if (! index.isValid()) {
	return Super::flags(index);
    }

    Qt::ItemFlags flags = Super::flags(index);
    if (index.column() == 1) {
	flags |= Qt::ItemIsEditable;
    }

    return flags;
}

bool
LoggerModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Logger::ProcLog log("setData", Log());
    LOGDEBUG2 << "index: " << index << " value: " << value.toInt() << " role: " << role << std::endl;

    if (index.isValid() && index.column() == 1 && role == Qt::EditRole) {
	auto priority = Logger::Priority::Level(value.toInt());
	LoggerTreeItem* item = getTreeItem(index);
	if (item) {
	    auto& log = item->getLog();
	    if (log.getPriorityLimit() != priority) {
		LOGDEBUG3 << "setting new value" << std::endl;
		log.setPriorityLimit(priority);
		QSettings settings;
		settings.beginGroup(kLoggerLogLevels);
		settings.setValue(item->getFullName(), value);
		emit dataChanged(index, index);
		return true;
	    }
	}
    }

    return Super::setData(index, value, role);
}

QModelIndex
LoggerModel::index(int row, int column, const QModelIndex& parent) const
{
    QModelIndex index;
    if (hasIndex(row, column, parent)) {
	auto parentNode = getTreeItem(parent);
	auto childNode = parentNode->getChild(row);
	if (childNode) {
	    index = createIndex(row, column, childNode);
        }
    }

    return index;
}

QModelIndex
LoggerModel::parent(const QModelIndex& index) const
{
    if (! index.isValid()) return QModelIndex();
    auto childNode = getTreeItem(index);
    if (childNode == root_) {
	return QModelIndex();
    }
    auto parentNode = childNode->getParent();
    if (parentNode == root_) {
	return QModelIndex();
    }
    return createIndex(parentNode->getIndex(), 0, parentNode);
}

LoggerTreeItem*
LoggerModel::getTreeItem(const QModelIndex& index) const
{
    LoggerTreeItem* node = 0;
    if (index.isValid()) {
	node = static_cast<LoggerTreeItem*>(index.internalPointer());
    }
    if (! node) {
	node = root_;
    }
    return node;
}

void
LoggerModel::dump() const
{
    root_->dump(std::clog, 0);
}
