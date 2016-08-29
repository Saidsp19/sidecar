#include <sstream>

#include "QtCore/QDateTime"
#include "QtGui/QMessageBox"
#include "QtGui/QTextCursor"
#include "QtGui/QTextDocument"

#include "GUI/ServiceEntry.h"
#include "GUI/LogUtils.h"

#include "Logger/Msg.h"
#include "Logger/Priority.h"
#include "Runner/LogCollector.h"
#include "XMLRPC/XmlRpcValue.h"

#include "App.h"
#include "ConfigurationInfo.h"
#include "LogAlertsWindow.h"
#include "LogViewWindow.h"
#include "MainWindow.h"
#include "RecordingController.h"
#include "RecordingInfo.h"
#include "RunnerLog.h"

using namespace SideCar::GUI::Master;
using namespace SideCar::Runner;

RunnerLog::ActiveHash RunnerLog::active_;

Logger::Log&
RunnerLog::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.RunnerLog");
    return log_;
}

RunnerLog*
RunnerLog::Find(const QString& serviceName)
{
    static Logger::ProcLog log("Find", Log());
    LOGINFO << "serviceName: " << serviceName << std::endl;
    ActiveHash::const_iterator pos = active_.find(serviceName);
    return pos != active_.end() ? pos.value() : 0;
}

void
RunnerLog::CleanUp()
{
    foreach (RunnerLog* rlp, active_.values())
	delete rlp;
    active_.clear();
}

void
RunnerLog::Make(const QString& name, const QString& configName,
                const QString& serviceName)
{
    static Logger::ProcLog log("Make", Log());
    LOGINFO << "name: " << name << std::endl;
    ActiveHash::const_iterator pos = active_.find(serviceName);
    if (pos != active_.end()) return;
    active_.insert(serviceName,
                   new RunnerLog(name, configName, serviceName));
}

RunnerLog::RunnerLog(const QString& name, const QString& configName,
                     const QString& serviceName)
    : QObject(App::GetApp()), name_(name), serviceName_(serviceName),
      alertPrefix_(configName), window_(0),
      logData_(new QTextDocument(this)), appendCursor_(logData_),
      lastMatch_(0), matcher_("^.*\\b(FATAL|ERROR|WARNING)\\b.*$")
{
    Logger::ProcLog log("RunnerLog", Log());;
    LOGINFO << "key: " << name << std::endl;

    alertPrefix_ += "/";
    alertPrefix_ += name;
    alertPrefix_ += ": ";

    logData_->setMaximumBlockCount(1000);
    matcher_.setMinimal(true);

    QFont font;
    font.setPointSize(10);
    logData_->setDefaultFont(font);
    logData_->setUndoRedoEnabled(false);

    appendCursor_.movePosition(QTextCursor::End);

    // Forward certain log messages to the application's LogAlertsWindow window and any recording notes window.
    //
    alertsWindow_ = App::GetApp()->getLogAlertsWindow();
    mainWindow_ = App::GetApp()->getMainWindow();

    connect(App::GetApp(), SIGNAL(shutdown()), SLOT(deleteLater()));
}

RunnerLog::~RunnerLog()
{
    Logger::ProcLog log("~RunnerLog", Log());
    active_.remove(serviceName_);
    delete window_;
    window_ = 0;
}

void
RunnerLog::showWindow()
{
    if (! window_)
	window_ = new LogViewWindow(name_ + " Log", logData_);
    window_->showAndRaise();
}

void
RunnerLog::appendData(const QString& data)
{
    static Logger::ProcLog log("appendData", Log());
    LOGINFO << "data: " << data << std::endl;

    if (window_)
	window_->beginUpdate();

    appendCursor_.insertText(data);
    appendCursor_.movePosition(QTextCursor::End);

    QTextCursor pos = logData_->find(matcher_, lastMatch_);
    LOGDEBUG << "pos: " << pos.position() << std::endl;

    QStringList found;
    while (! pos.isNull()) {
	found.append(alertPrefix_ + pos.selectedText() + '\n');
	lastMatch_ = pos.selectionEnd();
	pos = logData_->find(matcher_, lastMatch_);
    }

    if (! found.empty()) {
	alertsWindow_->addAlerts(found);
	RecordingInfo* recordingInfo =
	    mainWindow_->getRecordingController().getActiveRecording();
	if (recordingInfo)
	    recordingInfo->addAlerts(found);
    }
}

void
RunnerLog::appendLogMessages(const XmlRpc::XmlRpcValue& logMessages)
{
    static Logger::ProcLog log("appendLogMessages", Log());
    LOGINFO << "num logMessages: " << logMessages.size() << std::endl;

    if (logMessages.size() == 0)
	return;

    for (int index = 0; index < logMessages.size(); ++index) {
	const XmlRpc::XmlRpcValue& slot(logMessages[index]);
	QDateTime when = QDateTime::fromTime_t(
	    int(slot[LogCollector::kSeconds]));
	QString msg = QString("%1 %2 %3 %4")
	    .arg(when.toUTC().toString("hh:mm:ss"))
	    .arg(Logger::Priority::GetLongName(
                     Logger::Priority::Level(
                         int(slot[LogCollector::kPriorityLevel]))))
	    .arg(QString::fromStdString(slot[LogCollector::kChannel]))
	    .arg(QString::fromStdString(slot[LogCollector::kMessage]));
	appendData(msg);
    }
}
