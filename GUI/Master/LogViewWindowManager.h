#ifndef SIDECAR_GUI_MASTER_LOGVIEWWINDOWMANAGER_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_LOGVIEWWINDOWMANAGER_H

#include "QtCore/QObject"
#include "QtGui/QCheckBox"
#include "QtGui/QComboBox"

namespace Logger {
class Log;
}
namespace SideCar {
namespace Configuration {
class RunnerConfig;
}
namespace GUI {
namespace Master {

class RunnerItem;

/** Derivation of QComboBox that manages the set of known RunnerItem objects and their LogViewWindow windows.
    The QComboBox contains the runner names, segmented by their configuration names. Selecting a runner name
    will show and bring to the front the LogViewWindow window for that runner.
*/
class LogViewWindowManager : public QComboBox {
    Q_OBJECT
    using Super = QObject;

public:
    /** Obtain the log device for LogViewWindowManager objects

        \return Logger::Log device
    */
    static Logger::Log& Log();

    /** Constructor. Initializes an empty QComboBox object

        \param parent parent for auto-destruction
    */
    LogViewWindowManager(QWidget* parent = 0);

    /** Destructor. Delete the configuration mapping.
     */
    ~LogViewWindowManager();

    /** Add an entry for the given RunnerItem

        \param runnerItem RunnerItem to add
    */
    void addLogInfo(const RunnerItem* runnerItem);

    /** Add an entry for the given Configuration::RunnerConfig entry.

        \param runnerConfig RunnerConfig to add
    */
    void addLogInfo(const SideCar::Configuration::RunnerConfig* runnerConfig);

    /** Remove the entries associated with a specific configuration

        \param configName name of the configuration to remove
    */
    void removeConfiguration(const QString& configName);

private slots:

    /** Notification received when the user selects an entry from the QComboBox. Locate the appropriate
        LogViewWindow window and show/raise it.

        \param index entry of the QComboBox that the use selected
    */
    void widgetActivated(int index);

private:
    /** Add information about a particular remote runner process.

        \param name name of the remote runner

        \param configName name of the configuration defining the runner

        \param serviceName the service name the runner registered with Zeroconf
    */
    void addLogInfo(const QString& name, const QString& configName, const QString& serviceName);

    /** Create the items for the QComboBox.
     */
    void makeItems();

    struct ConfigMap;
    ConfigMap* configs_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
