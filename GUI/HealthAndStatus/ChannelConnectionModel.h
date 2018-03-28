#ifndef SIDECAR_GUI_HEALTHANDSTATUS_CHANNELCONNECTIONMODEL_H // -*- C++ -*-
#define SIDECAR_GUI_HEALTHANDSTATUS_CHANNELCONNECTIONMODEL_H

#include "QtCore/QAbstractListModel"
#include "QtCore/QList"

#include "GUI/ServiceBrowser.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace HealthAndStatus {

class ChannelConnection;

class ChannelConnectionModel : public QAbstractListModel {
    Q_OBJECT
    using Super = QAbstractListModel;

public:
    /** Obtain the log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    static ChannelConnection* GetObject(const QModelIndex& index)
    {
        return static_cast<ChannelConnection*>(index.internalPointer());
    }

    ChannelConnectionModel(QObject* parent = 0);

    ~ChannelConnectionModel();

    ChannelConnection* getConnection(int row) const { return connections_[row]; }

    QModelIndex add(ChannelConnection* channelConnection, int beforeRow);

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());

    void moveUp(int row);

    void moveDown(int row);

    int getRowFor(ChannelConnection* channelConnection) const { return connections_.indexOf(channelConnection); }

    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const;

    bool hasChannel(const QString& name) const;

signals:

    void availableServicesChanged(const QStringList& names);

private slots:

    void setAvailableServices(const ServiceEntryHash& services);

private:
    ChannelConnection* findChannelConnection(const QString& name) const;

    void swap(int row1, int row2);

    QList<ChannelConnection*> connections_;
    ServiceBrowser* browser_;
};

} // end namespace HealthAndStatus
} // end namespace GUI
} // end namespace SideCar

#endif
