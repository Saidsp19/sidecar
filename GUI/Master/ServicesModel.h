#ifndef SIDECAR_GUI_MASTER_SERVICESMODEL_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_SERVICESMODEL_H

#include "QtCore/QAbstractItemModel"
#include "QtCore/QByteArray"
#include "QtCore/QHash"
#include "QtCore/QList"
#include "QtCore/QStringList"

#include "IO/ProcessingState.h"
#include "GUI/ServiceBrowser.h"
#include "Time/TimeStamp.h"
#include "XMLRPC/XmlRpcValue.h"

namespace Logger { class Log; }
namespace XmlRpc { class XmlRpcValue; }

namespace SideCar {
namespace GUI {
namespace Master {

class ConfigurationItem;
class RootItem;
class RunnerItem;
class StatusCollector;
class TreeViewItem;

/** Model class for SideCar services discovered via Zeroconfig. Contains the concrete data that describes each
    service.
*/
class ServicesModel : public QAbstractItemModel
{
    Q_OBJECT
    using Super = QAbstractItemModel;
public:

    enum Columns {
	kName = 0,
	kHost,
	kState,
	kRecording,
	kPending,
	kRate,
	kError,
	kInfo,
	kNumColumns
    };

    static Logger::Log& Log();

    static QString GetColumnName(int index);

    static TreeViewItem* GetModelData(const QModelIndex& index);

    ServicesModel(QObject* parent);

    ~ServicesModel();

    bool start();

    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    int rowCount( const QModelIndex& parent = QModelIndex()) const;

    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex& index) const;

    bool postProcessingStateChange(const QStringList& configNames,
                                   IO::ProcessingState::Value) const;

    bool postClearStats(const QStringList& configNames) const;

    bool postRecordingStart(const QStringList& configNames,
                            const QStringList& recordingPaths) const;

    bool postRecordingStop(const QStringList& configNames) const;

    bool postShutdownRequest(const QString& configName) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;

    int getConfigurationCount() const;

    void getServiceStats(int& runnerCount, int& streamCount,
                         int& pendingCount, int& failureCount) const;

    void getDropsAndDupes(const QStringList& filter, int& drops,
                          int& dupes) const;

    bool isRecording(const QStringList& filter) const;

    bool isCalibrating(const QStringList& filter) const;

    ConfigurationItem* getConfigurationItem(const QString& name) const;

    QStringList getServiceNames(const QString& configName) const;

    RunnerItem* getRunnerItem(const QString& configName,
                              const QString& runnerName) const;

    /** Obtain a list of parameter settings that differ from their original settings as defined in the XML
        configuration files used to start Runner objects.

        \param definition XML container that will contain the changes

        \return true if successful
    */
    bool getChangedParameters(const QStringList& configNames,
                              QStringList& changes) const;

signals:

    /** Notification that the
     */
    void statusUpdated();

    void runnerAdded(const RunnerItem* added);

    void runnerRemoved(const RunnerItem* removed);

private slots:

    void foundServices(const ServiceEntryList& found);

    void lostServices(const ServiceEntryList& lost);

    void resolvedService(ServiceEntry* serviceEntry);

    void updateStatus(const QList<QByteArray>& statusReports);

private:

    QModelIndex getModelIndex(const TreeViewItem* configItem, int col = 0)
	const;

    bool postCommand(const QStringList& configNames, const char* cmd,
                     const XmlRpc::XmlRpcValue& args) const;

    bool postCommand(const QString& configName, const char* cmd,
                     const XmlRpc::XmlRpcValue& args) const;

    ServiceBrowser* browser_;
    StatusCollector* statusCollector_;
    RootItem* rootItem_;
    
    static const char* kColumnNames_[kNumColumns];
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
