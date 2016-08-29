#ifndef SIDECAR_GUI_MASTER_CONFIGURATIONMODEL_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_CONFIGURATIONMODEL_H

#include "QtCore/QAbstractTableModel"
#include "QtCore/QHash"
#include "QtCore/QList"

#include "ConfigurationInfo.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace Master {

class DiskSpaceMonitor;

class ConfigurationModel : public QAbstractTableModel
{
    Q_OBJECT
    using Super = QAbstractTableModel;
public:

    enum Columns {
	kName,
	kStatus,
	kViewable,
	kRecordable,
	kSpaceAvailable,
	kNumColumns
    };

    static Logger::Log& Log();

    static QString GetColumnName(int index);

    static ConfigurationInfo* GetModelData(const QModelIndex& index)
	{
	    return index.isValid() ?
		static_cast<ConfigurationInfo*>(index.internalPointer()) : 0;
	}

    ConfigurationModel(QObject* parent);

    ConfigurationInfo* getModelData(int row) const
	{ return configurationList_[row]; }

    ConfigurationInfo* getModelData(const QString& name) const;

    bool isAnyRecording() const;

    int getRowOf(ConfigurationInfo* obj) const;

    QModelIndex add(ConfigurationInfo* entry);

    void remove(int row);

    int columnCount(const QModelIndex& parent = QModelIndex()) const
	{ return parent.isValid() ? 0 : kNumColumns; }

    int rowCount(const QModelIndex& parent = QModelIndex()) const
	{ return parent.isValid() ? 0 : configurationList_.size(); }

    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const;

    bool setData(const QModelIndex& index, const QVariant& value,
                 int role = Qt::EditRole);

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;

    void statusChanged(ConfigurationInfo* obj);

    void spaceAvailableChanged(ConfigurationInfo* obj);

    void recordingStateChanged();

private:
    using ConfigurationList = QList<ConfigurationInfo*>;
    ConfigurationList configurationList_;
    using ConfigurationHash = QHash<QString,ConfigurationInfo*>;
    ConfigurationHash configurationHash_;
    DiskSpaceMonitor* diskSpaceMonitor_;

    static const char* kColumnNames_[kNumColumns];
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
