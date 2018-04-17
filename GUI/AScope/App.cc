#include "QtPrintSupport/QPrinter"
#include "QtWidgets/QMessageBox"

#include "GUI/LogUtils.h"
#include "GUI/Utils.h"
#include "Messages/Video.h"
#include "Utils/AzimuthSweep.h"

#include "App.h"
#include "ChannelConnectionWindow.h"
#include "Configuration.h"
#include "ConfigurationWindow.h"
#include "DisplayView.h"
#include "History.h"
#include "HistorySettings.h"
#include "MainWindow.h"
#include "ViewEditor.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::AScope;

Logger::Log&
App::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.App");
    return log_;
}

App::App(int& argc, char** argv) : AppBase("AScope", argc, argv), history_(new History(this)), windowCounter_(0)
{
    static Logger::ProcLog log("App", Log());
    LOGINFO << std::endl;

    printer_ = new QPrinter;
    printer_->setPageSize(QPrinter::Letter);
    printer_->setOrientation(QPrinter::Landscape);

    makeToolWindows();

    HistorySettings& historySettings = configuration_->getHistorySettings();
    history_->setEnabled(historySettings.isEnabled());
    history_->setDuration(historySettings.getDuration());
    connect(&historySettings, SIGNAL(enabledChanged(bool)), history_, SLOT(setEnabled(bool)));
    connect(&historySettings, SIGNAL(durationChanged(int)), history_, SLOT(setDuration(int)));
}

App::~App()
{
    delete printer_;
}

void
App::makeToolWindows()
{
    configurationWindow_ = new ConfigurationWindow(Qt::CTRL + Qt::Key_1 + kShowConfigurationWindow);
    addToolWindow(kShowConfigurationWindow, configurationWindow_);

    configuration_ = new Configuration(configurationWindow_);

    channelConnectionWindow_ = new ChannelConnectionWindow(Qt::CTRL + Qt::Key_1 + kShowChannelConnectionWindow);
    addToolWindow(kShowChannelConnectionWindow, channelConnectionWindow_);

    viewEditor_ = new ViewEditor(Qt::CTRL + Qt::Key_1 + kShowViewEditor);
    addToolWindow(kShowViewEditor, viewEditor_);
}

void
App::addDisplayView(DisplayView* displayView)
{
    static Logger::ProcLog log("addDisplayView", Log());
    LOGINFO << displayView << std::endl;

    connect(displayView, SIGNAL(activeDisplayViewChanged(DisplayView*)), viewEditor_,
            SLOT(activeDisplayViewChanged(DisplayView*)));

    connect(displayView, SIGNAL(activeDisplayViewChanged(DisplayView*)), channelConnectionWindow_,
            SLOT(activeDisplayViewChanged(DisplayView*)));
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    static Logger::ProcLog log("makeNewMainWindow", Log());
    LOGINFO << "objectName: " << objectName << std::endl;
    MainWindow* window = static_cast<MainWindow*>(getActiveMainWindow());
    DisplayView* displayView = window ? window->getActiveDisplayView() : 0;
    LOGDEBUG << window << ' ' << displayView << std::endl;
    return new MainWindow(*history_, ++windowCounter_, displayView);
}
