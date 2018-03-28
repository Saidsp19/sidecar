#include "QtCore/QMap"
#include "QtCore/QStringList"
#include "QtGui/QFont"
#include "QtGui/QItemDelegate"
#include "QtGui/QStyleOptionViewItem"

#include "Configuration/RunnerConfig.h"
#include "GUI/LogUtils.h"

#include "App.h"
#include "LogViewWindow.h"
#include "LogViewWindowManager.h"
#include "MainWindow.h"
#include "RunnerItem.h"
#include "RunnerLog.h"
#include "ServicesModel.h"

using namespace SideCar::GUI::Master;
using namespace SideCar::Configuration;

Logger::Log&
LogViewWindowManager::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.Master.LogViewWindowManager");
    return log_;
}

/** Internal 2-tuple object used by the LogViewWindowManager to hold the display and service names for a remote
    runner process.
*/
struct LogInfo {
    LogInfo() : displayName_(), serviceName_() {}

    LogInfo(const QString& displayName, const QString& serviceName) :
        displayName_(displayName), serviceName_(serviceName)
    {
    }

    bool operator<(const LogInfo& rhs) const { return displayName_ < rhs.displayName_; }

    bool operator==(const LogInfo& rhs) const { return displayName_ == rhs.displayName_; }

    bool operator!=(const LogInfo& rhs) const { return displayName_ != rhs.displayName_; }

    QString displayName_;
    QString serviceName_;
};

using LogInfoList = QList<LogInfo>;

/** Internal mapping from configuration names to a list of LogInfo entries. Used by LogViewWindowManager to
    locate the entries under a given configuration name.
*/
struct LogViewWindowManager::ConfigMap : public QMap<QString, LogInfoList> {
    using Super = QMap<QString, LogInfoList>;

    ConfigMap() : Super() {}
};

/** QComboBox view delegate. Renders entries in the QComboBox that represent configuration names with italic
    text.
*/
struct MyItemDelegate : public QItemDelegate {
    MyItemDelegate(LogViewWindowManager* parent) : QItemDelegate(parent), parent_(parent) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyleOptionViewItem option2(option);
        QVariant itemData = parent_->itemData(index.row());

        // Configuration entries in the QComboBox do not have any item data associated with them. Render the
        // text in italics.
        //
        if (itemData == QVariant::Invalid) {
            QFont font(option.font);
            font.setItalic(true);
            option2.font = font;
        }
        QItemDelegate::paint(painter, option2, index);
    }

    LogViewWindowManager* parent_;
};

LogViewWindowManager::LogViewWindowManager(QWidget* parent) : QComboBox(parent), configs_(new ConfigMap)
{
    connect(this, SIGNAL(activated(int)), SLOT(widgetActivated(int)));
    setItemDelegate(new MyItemDelegate(this));
}

LogViewWindowManager::~LogViewWindowManager()
{
    delete configs_;
}

void
LogViewWindowManager::addLogInfo(const RunnerConfig* runnerConfig)
{
    addLogInfo(runnerConfig->getRunnerName(), runnerConfig->getConfigurationName(), runnerConfig->getServiceName());
}

void
LogViewWindowManager::addLogInfo(const RunnerItem* runnerItem)
{
    addLogInfo(runnerItem->getName(), runnerItem->getConfigName(), runnerItem->getServiceName());
}

void
LogViewWindowManager::addLogInfo(const QString& runnerName, const QString& configName, const QString& serviceName)
{
    LogInfo logInfo(runnerName, serviceName);

    ConfigMap::iterator pos = configs_->find(configName);
    if (pos == configs_->end()) {
        // Insert a new mapping for the RunnerItem's configuration.
        //
        pos = configs_->insert(configName, LogInfoList());
    }

    if (pos.value().contains(logInfo)) return;

    // Create a new RunnerLog and add the LogInfo to the container.
    //
    RunnerLog::Make(runnerName, configName, serviceName);
    pos.value().append(logInfo);
    qSort(pos.value().begin(), pos.value().end());

    // Now rebuild our contents
    //
    makeItems();
}

void
LogViewWindowManager::makeItems()
{
    clear();

    // Iterate over the mapped configurations.
    //
    ConfigMap::const_iterator pos1 = configs_->begin();
    ConfigMap::const_iterator end1 = configs_->end();
    while (pos1 != end1) {
        // Add a divider to the QComboBox widget that contains the name of the configuration. Then add the
        // RunnerItem names.
        //
        QString configName(pos1.key());
        addItem(QString("- %1 -").arg(configName), QString());

        // Iterate over the LogInfo entries, using the displayName for the QCombBox name, and the serviceName
        // for the user data component.
        //
        LogInfoList::const_iterator pos2 = pos1.value().begin();
        LogInfoList::const_iterator end2 = pos1.value().end();
        while (pos2 != end2) {
            addItem(pos2->displayName_, pos2->serviceName_);
            ++pos2;
        }
        ++pos1;
    }

    setCurrentIndex(-1);
}

void
LogViewWindowManager::removeConfiguration(const QString& configName)
{
    ConfigMap::iterator pos1 = configs_->find(configName);
    if (pos1 == configs_->end()) return;

    // Delete all RunnerLog objects associated with the given configuration.
    //
    LogInfoList::const_iterator pos2 = pos1.value().begin();
    LogInfoList::const_iterator end2 = pos1.value().end();
    while (pos2 != end2) {
        delete RunnerLog::Find(pos2->serviceName_);
        ++pos2;
    }

    // Remove the mapping for the configuration name.
    //
    configs_->erase(pos1);

    // Recreate the QComboBox items.
    //
    makeItems();
}

void
LogViewWindowManager::widgetActivated(int index)
{
    Logger::ProcLog log("widgetActivated", Log());
    LOGINFO << "index: " << index << std::endl;
    if (index == -1) return;

    // Locate the service name associated with the item that was selected. Configuration items do not have a
    // service name, so we don't do anything when they are selected.
    //
    QString serviceName(itemData(index).toString());
    if (serviceName.size()) {
        LOGDEBUG << "serviceName: " << serviceName << std::endl;
        RunnerLog* runnerLog = RunnerLog::Find(serviceName);
        if (runnerLog) runnerLog->showWindow();
    }

    // Always unselect whatever was last selected.
    //
    setCurrentIndex(-1);
}
