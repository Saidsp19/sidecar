#include "ViewModel.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ExtractionEmitter;

ViewModel::ViewModel(QObject* parent)
    : QAbstractTableModel(parent),
      entries_(Messages::Extractions::Make("ExtractionEmitter",
                                           Messages::Header::Ref()))
{
    ;
}

void
ViewModel::add(double range, double azimuth)
{
    int rowIndex = rowCount();
    entries_->push_back(
	Messages::Extraction(Time::TimeStamp::Now(), range,
                             degreesToRadians(azimuth),
                             0.0));
    beginInsertRows(QModelIndex(), rowIndex, rowIndex);
    insertRows(rowIndex, 1);
    endInsertRows();
}

void
ViewModel::clear()
{
    entries_->getData().clear();
    beginRemoveRows(QModelIndex(), 0, rowCount());
    removeRows(0, rowCount());
    endRemoveRows();
}

int
ViewModel::columnCount(const QModelIndex& parent) const
{
    return 2;
}

int
ViewModel::rowCount(const QModelIndex& parent) const
{
    return entries_->size();
}

Qt::ItemFlags
ViewModel::flags(const QModelIndex& pos) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant
ViewModel::data(const QModelIndex& pos, int role) const
{
    if (! pos.isValid() ||
        pos.row() >= rowCount() ||
        pos.column() >= columnCount() ||
        role != Qt::DisplayRole) return QVariant();
    if (pos.column() == 0)
	return entries_[pos.row()].getRange();
    else
	return radiansToDegrees(entries_[pos.row()].getAzimuth());
}

bool
ViewModel::setData(const QModelIndex& pos, const QVariant& data, int role)
{
    if (! pos.isValid() ||
        pos.row() >= rowCount() ||
        pos.column() >= columnCount() ||
        role != Qt::EditRole) return false;
    if (pos.column() == 0) {
	entries_[pos.row()] =
	    Messages::Extraction(Time::TimeStamp::Now(), data.toDouble(),
                                 entries_[pos.row()].getAzimuth(),
                                 0.0);
    }
    else {
	entries_[pos.row()] =
	    Messages::Extraction(Time::TimeStamp::Now(),
                                 entries_[pos.row()].getRange(),
                                 degreesToRadians(data.toDouble()),
                                 0.0);
    }
    emit dataChanged(pos, pos);
    return true;
}

QVariant
ViewModel::headerData(int section, Qt::Orientation orientation, int role)
    const
{
    if (role != Qt::DisplayRole) return QVariant();
    if (orientation == Qt::Vertical) {
	if (section < rowCount()) {
	    return section + 1;
	}
	else {
	    return QVariant();
	}
    }
    return section ? "Azimuth" : "Range";
}

ViewModelChanger::ViewModelChanger(QObject* parent)
    : QObject(parent), model_(0), range_(0), azimuth_(0), row_(0)
{
    ;
}

void
ViewModelChanger::initialize(ViewModel* model, QSlider* range,
                             QSlider* azimuth)
{
    model_ = model;
    range_ = range;
    azimuth_ = azimuth;
    connect(range_, SIGNAL(valueChanged(int)),
            SLOT(updateRange(int)));
    connect(azimuth_, SIGNAL(valueChanged(int)),
            SLOT(updateAzimuth(int)));
}

void
ViewModelChanger::setRow(int row)
{
    if (row_ != row) {
	row_ = row;
	if (row >= 0 && row < model_->rowCount()) {
	    range_->setValue(
		model_->data(model_->index(row_, 0)).toInt());
	    azimuth_->setValue(
		model_->data(model_->index(row_, 1)).toInt());
	}
    }
}

void
ViewModelChanger::updateRange(int value)
{
    if (row_ >= 0 && row_ < model_->rowCount()) {
	QModelIndex index(model_->index(row_, 0));
	if (model_->data(index).toInt() != value)
	    model_->setData(index, value);
    }
}

void
ViewModelChanger::updateAzimuth(int value)
{
    if (row_ >= 0 && row_ < model_->rowCount()) {
	QModelIndex index(model_->index(row_, 1));
	if (model_->data(index).toInt() != value)
	    model_->setData(index, value);
    }
}
