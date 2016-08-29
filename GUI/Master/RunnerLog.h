#ifndef SIDECAR_GUI_MASTER_RUNNERLOG_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_RUNNERLOG_H

#include "QtCore/QHash"
#include "QtCore/QRegExp"
#include "QtGui/QTextCursor"

class QTextDocument;

namespace Logger { class Log; }
namespace XmlRpc { class XmlRpcValue; }

namespace SideCar {
namespace GUI {
namespace Master {

class LogAlertsWindow;
class LogViewWindow;
class MainWindow;

/** Log message collector object for a Runner applications. Runner applications can run anywhere on the SideCar
    LAN. While they run, they deposite log messages into a well-known file on their host machine:

    <DIR>/runner_<CONFIG>_<TAG>.log,

    where <DIR> is the value of the 'logOutputBasePath' XML attribute from the
    configuration file;  <CONFIG> is the name of the configuration
    file defining the Runner; and <TAG> is the optional 'tag' attribute value
    in the <b>host</b> stanza of the configuration file that generated the
    Runner instance.

    This class runs a <b>tail</b> command on host where the log file
    exists, and appends output from it to an internal QTextDocument object. A
    separate LogViewWindow object may be opened to view the contents of the
    QTextDocument object.

    The RunnerLog objects exist as singletons: once created, they never
    disappear from the Master application, since they can always access the
    remote log data. The RunnerLog restarts its log monitoring process when its
    restart() method is called. Currently, this is called when a new Runner
    process appears to the application's ServicesModel object, and there is
    already an active RunnerLog for the Runner's name / configuration
    tuple.
*/
class RunnerLog : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    /** Obtain the log device to use for RunnerLog objects.

        \return Log device
    */
    static Logger::Log& Log();

    /** 

        \param runnerName 

        \param configName 

        \return 
    */
    static RunnerLog* Find(const QString& serviceName);

    /** 

        \param runnerName 

        \param configName 

        \return 
    */
    static void Make(const QString& name, const QString& configName,
                     const QString& serviceName);

    static void CleanUp();

    /** Destructor. Closes and deletes any managed LogWindow.
     */
    ~RunnerLog();

    void appendLogMessages(const XmlRpc::XmlRpcValue& logMessages);

public slots:

    /** Show the log window. Creates a new LogViewWindow if none exits.
     */
    void showWindow();

private:

    /** Add log data to the internal QTextDocument. Scans the added data to see if it contains complete lines
        with a Logger priority value of WARNING or higher. It forwards any such lines found to the applications'
        LogAlertsWindow object.

        \param data text data to add
    */
    void appendData(const QString& data);

    /** Constructor. Restrict construction to the Find class method.
     */
    RunnerLog(const QString& name, const QString& configName,
              const QString& serviceName);

    QString name_;
    QString serviceName_;
    QString alertPrefix_;
    LogViewWindow* window_;
    QTextDocument* logData_;
    QTextCursor appendCursor_;
    int lastMatch_;
    QRegExp matcher_;
    LogAlertsWindow* alertsWindow_;
    MainWindow* mainWindow_;

    using ActiveHash = QHash<QString,RunnerLog*>;
    static ActiveHash active_;
    static int maxBufferSize_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
