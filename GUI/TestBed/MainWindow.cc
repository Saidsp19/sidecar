#include "QtCore/QTimer"

#include "GUI/LogUtils.h"

#include "MainWindow.h"

using namespace SideCar::GUI::TestBed;

Logger::Log&
MainWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("spectrum.MainWindow");
    return log_;
}

MainWindow::MainWindow() : MainWindowBase(), Ui::TestBed()
{
    static Logger::ProcLog log("MainWindow", Log());
    LOGINFO << std::endl;

    setupUi(this);
    setObjectName("MainWindow");
#ifdef __DEBUG__
    setWindowTitle("TestBed (DEBUG)");
#else
    setWindowTitle("TestBed");
#endif
}

void
MainWindow::on_actionA__clicked()
{
    ledA_->cycleColors();
}

void
MainWindow::on_actionB__clicked()
{
    ledB_->cycleColors();
}

void
MainWindow::on_actionC__clicked()
{
    ledC_->cycleColors();
}

void
MainWindow::on_actionD__clicked()
{
    ledD_->cycleColors();
}

void
MainWindow::on_modeA__clicked()
{
}

void
MainWindow::on_modeB__clicked()
{
}
