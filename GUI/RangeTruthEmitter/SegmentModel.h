#ifndef SIDECAR_GUI_SEGMENTMODEL_H // -*- C++ -*-
#define SIDECAR_GUI_SEGMENTMODEL_H

#include "QtCore/QAbstractTableModel"
#include "QtCore/QList"
#include "QtCore/QTextStream"

namespace SideCar {
namespace GUI {
namespace RangeTruthEmitter {

struct Segment {
    Segment() : xVel_(0.0), yVel_(0.0), zVel_(0.0), xAcc_(0.0), yAcc_(0.0), zAcc_(0.0), duration_(10.0), end_(0.0) {}
    double xVel_;     // m/s
    double yVel_;     // m/s
    double zVel_;     // m/s
    double xAcc_;     // m/s2
    double yAcc_;     // m/s2
    double zAcc_;     // m/s2
    double duration_; // seconds;
    double end_;
};

/** Model class for SideCar services discovered via Zeroconfig. Contains the concrete data that describes each
    service.
*/
class SegmentModel : public QAbstractTableModel {
    Q_OBJECT
    using Super = QAbstractTableModel;

public:
    enum Columns { kDuration = 0, kXVel, kYVel, kZVel, kXAcc, kYAcc, kZAcc, kEnd, kNumColumns };

    static QString GetColumnName(int index);

    SegmentModel(QObject* parent);

    ~SegmentModel();

    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    void addRow(int row);

    void addRow(Segment* segment);

    void moveUp(int row);

    void moveDown(int row);

    void deleteRow(int row);

    void setRunning(bool state) { running_ = state; }

    void clear();

    void rewind() { setLastActiveRow(-1); }

    Segment* getActiveSegment(double elapsed);

    void save(QTextStream& out) const;

signals:

    void segmentChanged();

private:
    void setLastActiveRow(int row);
    void updateEnds();

    QList<Segment*> segments_;
    int lastActiveRow_;
    bool running_;

    static const char* kColumnNames_[kNumColumns];
};

} // namespace RangeTruthEmitter
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
