#ifndef SIDECAR_GUI_LOGGERMODEL_H // -*- C++ -*-
#define SIDECAR_GUI_LOGGERMODEL_H

#include "QtCore/QAbstractItemModel"
#include "QtCore/QHash"
#include "QtCore/QString"

namespace Logger { class Log; }
namespace SideCar {
namespace GUI {

class LoggerTreeItem;

class LoggerModel : public QAbstractItemModel
{
    Q_OBJECT
    using Super = QAbstractItemModel;
public:

    using ItemHash = QHash<QString,LoggerTreeItem*>;

    static Logger::Log& Log();
    
    LoggerModel(QObject* parent = 0);

    ~LoggerModel();

    void initialize();

    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    QVariant data(const QModelIndex& index, int role) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;

    bool setData(const QModelIndex& index, const QVariant& value,
                 int role = Qt::EditRole);

    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex& index) const;

    void dump() const;

signals:

    void recordNewDevice(const QString& fullName);

private slots:

    void doRecordNewDevice(const QString& fulllName);
    
private:

    LoggerTreeItem* getTreeItem(const QModelIndex& index) const;

    LoggerTreeItem* addDevice(const std::string& name);

    void newDevice(Logger::Log& log);

    LoggerTreeItem* makeTreeItem(Logger::Log& log, LoggerTreeItem* parent);

    ItemHash hash_;
    LoggerTreeItem* root_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
