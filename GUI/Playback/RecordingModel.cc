#include "QtGui/QIcon"

#include "GUI/LogUtils.h"

#include "RecordingModel.h"

using namespace SideCar::GUI::Playback;

const char* RecordingModel::kColumnNames_[] = {
    "Recording",
    "Start",
    "Duration",
    "Drops",
    "T?",
    "MHz",
    "R?",
    "RPM",
    "DRFM?",
    "DRFM Cfg",
    "XML Cfgs",
};

Logger::Log&
RecordingModel::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("playback.RecordingModel");
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
    int row = recordingList_.size();
    beginInsertRows(QModelIndex(), row, row);
    recordingList_.append(info);
    endInsertRows();
    emit dataChanged(index(row, 0), index(row, kDRFMConfig));
}

void
RecordingModel::clear()
{
    if (recordingList_.empty()) return;
    beginRemoveRows(QModelIndex(), 0, recordingList_.size() - 1);
    while (! recordingList_.empty())
	delete recordingList_.takeLast();
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
	case kConfig:
	    value = int(Qt::AlignVCenter | Qt::AlignLeft);
	    break;
	default:
	    value = int(Qt::AlignCenter);
	    break;
	}
	break;

    case Qt::ToolTipRole:
	switch (index.column()) {
	case kName:
	    value = obj->getRecordingDirectory();
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
	case kStartTime:
	    value = obj->getStartTime();
	    break;
	case kElapsed:
	    value = obj->getElapsedTime();
	    break;
	case kDrops:
	    value = obj->getDropCount();
	    break;
	case kTransmitting:
	    if (obj->wasRadarTransmitting()) value = "Y";
	    break;
	case kFrequency:
	    value = obj->getRadarFreqency();
	    break;
	case kRotating:
	    if (obj->wasRadarRotating()) value = "Y";
	    break;
	case kRotationRate:
	    value = obj->getRadarRotationRate();
	    break;
	case kDRFMOn:
	    if (obj->wasDRFMOn()) value = "Y";
	    break;
	case kDRFMConfig:
	    value = obj->getDRFMConfig();
	    break;
	case kConfig:
	    value = obj->getConfigurationNames().join(", ").trimmed();
	    break;
	default: break;
	}
	break;

    default:
	break;
    }

    return value;
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
