#ifndef SIDECAR_GUI_VIEWMODEL_H	// -*- C++ -*-
#define SIDECAR_GUI_VIEWMODEL_H

#include <cmath>

#include "QtCore/QAbstractTableModel"
#include "QtGui/QSlider"

#include "Messages/Extraction.h"

namespace SideCar {
namespace GUI {
namespace ExtractionEmitter {

template <typename T> T degreesToRadians(T v) { return v * M_PI_4 / 45.0; }
template <typename T> T radiansToDegrees(T v) { return v * 45.0 / M_PI_4; }

class ViewModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ViewModel(QObject* parent);

    void add(double range, double azimuth);
    void clear();
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex& pos) const;
    QVariant data(const QModelIndex& pos, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex& pos, const QVariant& data,
                 int role = Qt::EditRole);
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    const Messages::Extractions::Ref getEntries() const { return entries_; }

private:
    Messages::Extractions::Ref entries_;
};

class ViewModelChanger : public QObject
{
    Q_OBJECT
public:
    ViewModelChanger(QObject* parent = 0);
    void initialize(ViewModel* model, QSlider* range, QSlider* azimuth);
    void setRow(int row);

private slots:
    void updateRange(int value);
    void updateAzimuth(int value);

private:
    ViewModel* model_;
    QSlider* range_;
    QSlider* azimuth_;
    int row_;
};

} // end namespace ExtractionEmitter
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
