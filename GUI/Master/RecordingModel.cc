#include "GUI/LogUtils.h"

#include "RecordingModel.h"
#include "TreeViewItem.h"

using namespace SideCar::GUI::Master;

const char* RecordingModel::kColumnNames_[] = {
    "Name",
    "Configs",
    "Start",
    "Duration",
    "Drops",
    "T?",
    "MHz",
    "R?",
    "RPM",
    "D?",
    "DRFM Config"
};

Logger::Log&
RecordingModel::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.RecordingModel");
    return log_;
}

QString
RecordingModel::GetColumnName(int index)
{
    return QString(kColumnNames_[index]);
}

RecordingModel::RecordingModel(QObject* parent)
    : QAbstractTableModel(parent), recordingList_()
{
    ;
}

int
RecordingModel::getRowOf(RecordingInfo* obj) const
{
    return recordingList_.indexOf(obj);
}


void
RecordingModel::add(RecordingInfo* info)
{
    connect(info, SIGNAL(statsChanged()),
            SLOT(recordingInfoStatsChanged()));
    connect(info, SIGNAL(configChanged()),
            SLOT(recordingInfoConfigChanged()));
    int row = recordingList_.size();
    beginInsertRows(QModelIndex(), row, row);
    recordingList_.append(info);
    endInsertRows();
    emit dataChanged(index(row, 0), index(row, kDRFMConfig));
}

void
RecordingModel::remove(const QModelIndex& index)
{
    Q_ASSERT(index.isValid());
    int row = index.row();
    beginRemoveRows(QModelIndex(), row, row);
    delete recordingList_.takeAt(row);
    endRemoveRows();
}

QModelIndex
RecordingModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || row >= recordingList_.size() ||
        column < 0 || column >= kNumColumns || parent.isValid())
	return QModelIndex();
    return createIndex(row, column, recordingList_[row]);
}

QVariant
RecordingModel::headerData(int section, Qt::Orientation orientation,
                           int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    if (orientation == Qt::Vertical) return QVariant();
    return GetColumnName(section);
}

QVariant
RecordingModel::data(const QModelIndex& index, int role) const
{
    static const QIcon kCheckIcon(":/checkMark.png");
    QVariant value;
  
    if (! index.isValid())
	return value;
  
    RecordingInfo* obj = getModelData(index.row());
    switch (role) {
	
    case Qt::TextAlignmentRole:
	switch (index.column()) {
	case kName:
	case kDRFMConfig:
	    value = int(Qt::AlignVCenter | Qt::AlignLeft);
	    break;
	default:
	    value = int(Qt::AlignCenter);
	    break;
	}
	break;
	
    case Qt::ForegroundRole:
	value = obj->isDone() ? Qt::black : TreeViewItem::GetRecordingColor();
	break;
	  
    case Qt::DecorationRole:
	switch (index.column()) {
	case kTransmitting:
	    if (obj->wasRadarTransmitting()) value = kCheckIcon;
	    break;
	case kRotating:
	    if (obj->wasRadarRotating()) value = kCheckIcon;
	    break;
	case kDRFMOn:
	    if (obj->wasDRFMOn()) value = kCheckIcon;
	    break;
	default: break;
	}
	break;
	
    case Qt::ToolTipRole:
	switch (index.column()) {
	case kName:
	    value = obj->getRecordingDirectories().join("<br>");
	    break;
	case kDrops:
	    value = QString("Number of dropped messages");
	    break;
	case kTransmitting:
	    value = QString(obj->wasRadarTransmitting() ?
                            "Radar was transmitting" :
                            "Radar was not transmitting");
	    break;
	case kFrequency:
	    if (obj->wasRadarTransmitting())
		value = QString("Radar transmitter frequency in MHz");
	    break;
	case kRotating:
	    value = QString(obj->wasRadarRotating() ?
                            "Radar was rotating" :
                            "Radar was not rotating");
	    break;
	case kRotationRate:
	    if (obj->wasRadarRotating())
		value = QString("Shaft rotation speed in RPM");
	    break;
	case kDRFMOn:
	    value = QString(obj->wasDRFMOn() ?
                            "LL DRFM was operational" :
                            "LL DRFM was not operational");
	    break;
	default:
	    break;
	}
	break;
	
    case Qt::DisplayRole:
	switch (index.column()) {
	case kName:
	    value = obj->getName();
	    break;
	case kConfig:
	    value = obj->getConfigurationNames().join("\n").trimmed();
	    break;
	case kStartTime:
	    value = obj->getStartTime();
	    break;
	case kElapsed:
	    value = obj->getElapsedTime();
	    break;
	case kDrops:
	    value = obj->getDropCount();
	    break;
	case kFrequency:
	    value = obj->getRadarFreqency();
	    break;
	case kRotationRate:
	    value = obj->getRadarRotationRate();
	    break;
	case kDRFMConfig:
	    value = obj->getDRFMConfig();
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

void
RecordingModel::updateDuration()
{
    int row = recordingList_.size() - 1;
    if (row >= 0)
	emit dataChanged(index(row, kElapsed), index(row, kElapsed));
}

void
RecordingModel::recordingInfoStatsChanged()
{
    RecordingInfo* info = static_cast<RecordingInfo*>(sender());
    if (! info) return;
    int row = getRowOf(info);
    if (row == -1) return;
    emit dataChanged(index(row, kDrops), index(row, kDrops));
}

void
RecordingModel::recordingInfoConfigChanged()
{
    RecordingInfo* info = static_cast<RecordingInfo*>(sender());
    if (! info) return;
    int row = getRowOf(info);
    if (row == -1) return;
    emit dataChanged(index(row, kTransmitting),
                     index(row, kNumColumns - 1));
}

RecordingInfo*
RecordingModel::getModelData(const QString& name) const
{
    for (int index = 0; index < rowCount(); ++index) {
	RecordingInfo* info = getModelData(index);
	if (info->getName() == name)
	    return info;
    }

    return 0;
}
