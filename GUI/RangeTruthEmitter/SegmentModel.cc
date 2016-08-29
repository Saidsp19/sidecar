#include "SegmentModel.h"

using namespace SideCar::GUI::RangeTruthEmitter;

const char* SegmentModel::kColumnNames_[] = {
    "Duration (s)",
    "Vel X (m/s)",
    "Vel Y (m/s)",
    "Vel Z (m/s)",
    "Acc X (m/s2)",
    "Acc Y (m/s2)",
    "Acc Z (m/s2)",
    "End (s)"
};

QString
SegmentModel::GetColumnName(int index)
{
    return QString(kColumnNames_[index]);
}

SegmentModel::SegmentModel(QObject* parent)
    : Super(parent), segments_(), lastActiveRow_(-1), running_(false)
{
    ;
}

SegmentModel::~SegmentModel()
{
    while (! segments_.empty()) {
	delete segments_.takeLast();
    }
}

int
SegmentModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : kNumColumns;
}

int
SegmentModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : segments_.size();
}

QVariant
SegmentModel::headerData(int section, Qt::Orientation orientation,
                         int role) const
{
    if (orientation == Qt::Vertical) {
	if (role == Qt::DisplayRole)
	    return QString::number(section);
    }
    else {
	if (role == Qt::TextAlignmentRole)
	    return int(Qt::AlignVCenter | Qt::AlignHCenter);
	if (role == Qt::DisplayRole)
	    return QString(GetColumnName(section));
    }
    
    return QVariant();
}

QVariant
SegmentModel::data(const QModelIndex& index, int role) const
{
    QVariant v;
 
    if (! index.isValid()) return v;
    
    if (role == Qt::BackgroundRole && index.row() == lastActiveRow_) {
	v = Qt::yellow;
    }
    else if (role == Qt::DisplayRole || role == Qt::EditRole) {
	Segment* segment = segments_[index.row()];
	switch (index.column()) {
	case kDuration: v = segment->duration_; break;
	case kXVel: v = segment->xVel_; break;
	case kYVel: v = segment->yVel_; break;
	case kZVel: v = segment->zVel_; break;
	case kXAcc: v = segment->xAcc_; break;
	case kYAcc: v = segment->yAcc_; break;
	case kZAcc: v = segment->zAcc_; break;
	case kEnd:  v = segment->end_;  break;
	default: break;
	}
    }

    return v;
}

Qt::ItemFlags
SegmentModel::flags(const QModelIndex& index) const
{
    if (! index.isValid())
	return Super::flags(index);

    if (running_ || index.column() == kEnd)
	return Qt::NoItemFlags;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

bool
SegmentModel::setData(const QModelIndex& index, const QVariant& value,
                      int role)
{
    if (! index.isValid() || role != Qt::EditRole)
	return false;

    bool ok = false;
    double v = value.toDouble(&ok);
    if (! ok)
	return false;

    Segment* segment = segments_[index.row()];
    switch (index.column()) {
    case kDuration:
	if (v <= 0)
	    return false;
	if (v != segment->duration_) {
	    segment->duration_ = v;
	    emit segmentChanged();
	}
	updateEnds();
	emit dataChanged(this->index(0, kEnd),
                         this->index(rowCount() - 1, kEnd));
	break;

    case kXVel:
	if (v != segment->xVel_) {
	    segment->xVel_ = v;
	    emit segmentChanged();
	    emit dataChanged(index, index);
	}
	break;

    case kYVel:
	if (v != segment->yVel_) {
	    segment->yVel_ = v;
	    emit segmentChanged();
	    emit dataChanged(index, index);
	}
	break;

    case kZVel:
	if (v != segment->zVel_) {
	    segment->zVel_ = v;
	    emit segmentChanged();
	    emit dataChanged(index, index);
	}
	break;

    case kXAcc:
	if (v != segment->xAcc_) {
	    segment->xAcc_ = v;
	    emit segmentChanged();
	    emit dataChanged(index, index);
	}
	break;

    case kYAcc:
	if (v != segment->yAcc_) {
	    segment->yAcc_ = v;
	    emit segmentChanged();
	    emit dataChanged(index, index);
	}
	break;

    case kZAcc:
	if (v != segment->zAcc_) {
	    segment->zAcc_ = v;
	    emit segmentChanged();
	    emit dataChanged(index, index);
	}
	break;

    default:
	return false;
    }

    return true;
}

void
SegmentModel::addRow(int row)
{
    Segment* segment = new Segment;
    beginInsertRows(QModelIndex(), row, row);
    segments_.insert(row, segment);
    updateEnds();
    endInsertRows();
}

void
SegmentModel::addRow(Segment* segment)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    segments_.append(segment);
    updateEnds();
    endInsertRows();
}

void
SegmentModel::moveUp(int row)
{
    Segment* tmp = segments_[row - 1];
    segments_[row - 1] = segments_[row];
    segments_[row] = tmp;
    updateEnds();
    emit dataChanged(index(row - 1, 0), index(row, kEnd));
}

void
SegmentModel::moveDown(int row)
{
    Segment* tmp = segments_[row + 1];
    segments_[row + 1] = segments_[row];
    segments_[row] = tmp;
    updateEnds();
    emit dataChanged(index(row, 0), index(row + 1, kEnd));
}

void
SegmentModel::deleteRow(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    delete segments_.takeAt(row);
    updateEnds();
    endRemoveRows();
}

void
SegmentModel::updateEnds()
{
    setLastActiveRow(-1);
    double end = 0.0;
    for (int index = 0; index < segments_.size(); ++index) {
	end += segments_[index]->duration_;
	segments_[index]->end_ = end;
    }
}

Segment*
SegmentModel::getActiveSegment(double elapsed)
{
    if (lastActiveRow_ == -1) {
	for (int row = 0; row < segments_.size(); ++row) {
	    if (segments_[row]->end_ >= elapsed) {
		setLastActiveRow(row);
		break;
	    }
	}
    }
    else {
	int row = lastActiveRow_;
	while (segments_[row]->end_ < elapsed) {
	    ++row;
	    if (row == segments_.size()) {
		row = -1;
		break;
	    }
	}

	if (row != lastActiveRow_)
	    setLastActiveRow(row);
    }

    return lastActiveRow_ == -1 ? 0 : segments_[lastActiveRow_];
}

void
SegmentModel::setLastActiveRow(int row)
{
    if (row != lastActiveRow_) {
	if (lastActiveRow_ != -1) {
	    emit dataChanged(index(lastActiveRow_, 0),
                             index(lastActiveRow_, kEnd));
	}

	lastActiveRow_ = row;
	
	if (lastActiveRow_ != -1) {
	    emit dataChanged(index(lastActiveRow_, 0),
                             index(lastActiveRow_, kEnd));
	}
    }
}

void
SegmentModel::clear()
{
    if (! segments_.empty()) {
	beginRemoveRows(QModelIndex(), 0, segments_.size() - 1);
	while (! segments_.empty()) {
	    delete segments_.takeLast();
	}
	endRemoveRows();
    }

    setLastActiveRow(-1);
}

void
SegmentModel::save(QTextStream& out) const
{
    for (int index = 0; index < segments_.size(); ++index) {
	const Segment* seg = segments_[index];
	out << seg->xVel_ << ' ' << seg->xAcc_ << ' '
	    << seg->yVel_ << ' ' << seg->yAcc_ << ' '
	    << seg->zVel_ << ' ' << seg->zAcc_ << ' '
	    << seg->duration_ << '\n';
    }
}
