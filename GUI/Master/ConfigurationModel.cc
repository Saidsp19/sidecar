#include "GUI/LogUtils.h"

#include "App.h"
#include "ConfigurationInfo.h"
#include "ConfigurationModel.h"
#include "TreeViewItem.h"

using namespace SideCar::GUI::Master;

const char* ConfigurationModel::kColumnNames_[] = {
    "Name",
    "Status",
    "View",
    "Rec",
    "Disk"
};

Logger::Log&
ConfigurationModel::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.ConfigurationModel");
    return log_;
}

QString
ConfigurationModel::GetColumnName(int index)
{
    return QString(kColumnNames_[index]);
}

ConfigurationModel::ConfigurationModel(QObject* parent)
    : QAbstractTableModel(parent), configurationList_(), configurationHash_()
{
    ;
}

int
ConfigurationModel::getRowOf(ConfigurationInfo* obj) const
{
    return configurationList_.indexOf(obj);
}


ConfigurationInfo*
ConfigurationModel::getModelData(const QString& name) const
{
    ConfigurationHash::const_iterator pos = configurationHash_.find(name);
    return pos != configurationHash_.end() ? *pos : 0;
}

QModelIndex
ConfigurationModel::add(ConfigurationInfo* info)
{
    ConfigurationList::iterator pos = configurationList_.begin();
    ConfigurationList::iterator end = configurationList_.end();

    const QString& name = info->getName();
    int row = 0;
    while (pos != end) {
	int rc = QString::compare((*pos)->getName(), name,
                                  Qt::CaseInsensitive);
	if (rc >= 0) break;
	++row;
	++pos;
    }

    beginInsertRows(QModelIndex(), row, row);

    if (pos == end)
	configurationList_.append(info);
    else
	configurationList_.insert(pos, info);
    configurationHash_[name] = info;

    endInsertRows();

    emit dataChanged(index(row, 0), index(row, kSpaceAvailable));

    return index(row, 0);
}

void
ConfigurationModel::remove(int row)
{
    ConfigurationInfo* obj = getModelData(row);
    Q_ASSERT(obj);
    beginRemoveRows(QModelIndex(), row, row);
    configurationHash_.remove(obj->getName());
    delete configurationList_.takeAt(row);
    endRemoveRows();
}

QModelIndex
ConfigurationModel::index(int row, int column,
                          const QModelIndex& parent) const
{
    if (row < 0 || row >= configurationList_.size() ||
        column < 0 || column >= kNumColumns || parent.isValid())
	return QModelIndex();
    return createIndex(row, column, configurationList_[row]);
}

QVariant
ConfigurationModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (role != Qt::DisplayRole)
	return Super::headerData(section, orientation, role);
    if (orientation == Qt::Vertical)
	return QVariant();
    return GetColumnName(section);
}

QVariant
ConfigurationModel::data(const QModelIndex& index, int role) const
{
    QVariant value;

    if (! index.isValid())
	return value;

    ConfigurationInfo* obj = getModelData(index.row());
    switch (role) {
    case Qt::TextAlignmentRole:
	switch (index.column()) {

	case kStatus:
	case kViewable:
	case kRecordable:
	    value = int(Qt::AlignCenter);
	    break;

	default:
	    value = int(Qt::AlignLeft | Qt::AlignVCenter);
	    break;
	}
	break;

    case Qt::ForegroundRole:
	switch (index.column()) {
	case kName:
	case kStatus:
	    if (obj->getStatus() == ConfigurationInfo::kPartial ||
                obj->hasError())
		value = TreeViewItem::GetFailureColor();
	    break;

	case kSpaceAvailable:
	    if (obj->getRecordingDirPercentUsed() > 0.90)
		value = TreeViewItem::GetFailureColor();
	    break;

	default: break;
	}
	break;

    case Qt::DisplayRole:
	switch (index.column()) {

	case kName:
	    value = obj->getName();
	    break;

	case kStatus:
	    value = obj->getStatusText();
	    break;

	case kSpaceAvailable:
	    value = obj->getRecordingDirFreeSpace();
	    break;

	default:
	    break;
	}
	break;

    case Qt::ToolTipRole:
	switch (index.column()) {

	case kName:
	    value = obj->getPath();
	    break;

	case kStatus:
	    if (obj->hasError())
		value = obj->getErrorText();
	    break;

	case kViewable:
	    value = QString("Click to %1 remote processes in the "
                            "view below").arg(obj->isViewable() ?
                                              "hide" : "show");
	    break;

	case kRecordable:
	    value = QString("Click to %1 recording of the configuration "
                            "processes").arg(obj->isRecordable() ?
                                             "disable" : "enable");
	    break;

	case kSpaceAvailable:
	    value = obj->getRecordingsDirectory();
	    break;

	default:
	    break;
	};
	break;

    case Qt::CheckStateRole:
	switch (index.column()) {
	case kViewable:
	    value = obj->isViewable() ? Qt::Checked : Qt::Unchecked;
	    break;
	case kRecordable:
	    value = obj->canRecord() ? Qt::Checked : Qt::Unchecked;
	    break;
	default:
	    break;
	}
	break;

    default:
	break;
    }
    return value;
}

bool
ConfigurationModel::setData(const QModelIndex& index, const QVariant& value,
                            int role)
{
    if (! index.isValid())
	return false;

    ConfigurationInfo* obj = getModelData(index.row());
    switch (index.column()) {
    case kViewable:
	obj->setViewable(value.toBool());
	emit dataChanged(index, index);
	return true;

    case kRecordable:
	obj->setRecordable(value.toBool());
	emit dataChanged(index, index);
	return true;

    default:
	break;
    }

    return false;
}

Qt::ItemFlags
ConfigurationModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = Super::flags(index);
    if (! index.isValid())
	return flags;

    ConfigurationInfo* obj = getModelData(index.row());

    switch (index.column()) {

    case kViewable:
	flags |= Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
	break;

    case kRecordable:
	flags = Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
	if (obj->hasValidRecordingDirectory())
	    flags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	break;

    default:
	break;
    }

    return flags;
}

void
ConfigurationModel::statusChanged(ConfigurationInfo* obj)
{
    int row = getRowOf(obj);
    if (row != -1) {
	emit dataChanged(index(row, 0), index(row, kStatus));
    }
}

void
ConfigurationModel::spaceAvailableChanged(ConfigurationInfo* obj)
{
    int row = getRowOf(obj);
    if (row != -1) {
	emit dataChanged(index(row, kSpaceAvailable),
                         index(row, kSpaceAvailable));
    }
}

void
ConfigurationModel::recordingStateChanged()
{
    if (rowCount())
	emit dataChanged(index(0, 0),
                         index(rowCount() - 1, kNumColumns - 1));
}

bool
ConfigurationModel::isAnyRecording() const
{
    foreach(ConfigurationInfo* cfg, configurationList_) {
	if (cfg->isRecording()) return true;
    }

    return false;
}
