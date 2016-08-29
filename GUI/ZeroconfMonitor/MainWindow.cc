#include "QtGui/QStatusBar"

#include "GUI/LogUtils.h"
#include "IO/ZeroconfRegistry.h"

#include "MainWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ZeroconfMonitor;

Logger::Log&
MainWindow::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("zcm.MainWindow");
    return log_;
}

MainWindow::MainWindow()
    : MainWindowBase(), Ui::MainWindow()
{
    static Logger::ProcLog log("MainWindow", Log());
    LOGINFO << std::endl;

    setupUi(this);

    publishers_->setLabel(publishersLabel_);
    controllers_->setLabel(controllersLabel_);
    collectors_->setLabel(collectorsLabel_);

    setAttribute(Qt::WA_DeleteOnClose);

#ifdef __DEBUG__
    setWindowTitle("Zeroconf Monitor (DEBUG)");
#else
    setWindowTitle("Zeroconf Monitor");
#endif

    publishers_->start(IO::ZeroconfRegistry::GetType(
                           IO::ZeroconfRegistry::kPublisher));
    controllers_->start(IO::ZeroconfRegistry::GetType(
                            IO::ZeroconfRegistry::kRunnerRemoteController));
    collectors_->start(IO::ZeroconfRegistry::GetType(
                           IO::ZeroconfRegistry::kRunnerStatusCollector));
}
