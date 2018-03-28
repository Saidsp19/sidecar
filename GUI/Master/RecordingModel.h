#ifndef SIDECAR_GUI_MASTER_RECORDINGMODEL_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_RECORDINGMODEL_H

#include "QtCore/QAbstractTableModel"
#include "QtCore/QList"
#include "QtGui/QIcon"

#include "RecordingInfo.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Master {

class RecordingModel : public QAbstractTableModel {
    Q_OBJECT
    using Super = QAbstractTableModel;

public:
    enum Columns {
        kName,
        kConfig,
        kStartTime,
        kElapsed,
        kDrops,
        kTransmitting,
        kFrequency,
        kRotating,
        kRotationRate,
        kDRFMOn,
        kDRFMConfig,
        kNumColumns
    };

    static Logger::Log& Log();

    static QString GetColumnName(int index);

    static RecordingInfo* GetModelData(const QModelIndex& index)
    {
        return index.isValid() ? static_cast<RecordingInfo*>(index.internalPointer()) : 0;
    }

    RecordingModel(QObject* parent);

    RecordingInfo* getModelData(const QString& name) const;

    RecordingInfo* getModelData(int row) const { return row >= 0 ? recordingList_[row] : 0; }

    int getRowOf(RecordingInfo* obj) const;

    void add(RecordingInfo* entry);

    void remove(const QModelIndex& index);

    int columnCount(const QModelIndex& parent = QModelIndex()) const { return parent.isValid() ? 0 : kNumColumns; }

    int rowCount(const QModelIndex& parent = QModelIndex()) const
    {
        return parent.isValid() ? 0 : recordingList_.size();
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void updateDuration();

private slots:

    void recordingInfoConfigChanged();

    void recordingInfoStatsChanged();

private:
    using RecordingList = QList<RecordingInfo*>;
    RecordingList recordingList_;

    static const char* kColumnNames_[kNumColumns];
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
