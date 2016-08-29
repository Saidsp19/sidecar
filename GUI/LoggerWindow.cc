#include "LoggerModel.h"
#include "LoggerWindow.h"
#include "modeltest.h"
#include "PriorityWidget.h"

#include "ui_LoggerWindow.h"

using namespace SideCar;
using namespace SideCar::GUI;

LoggerWindow::LoggerWindow(int shortcut)
    : Super("LoggerWindow", "Logger Configuration", shortcut),
      gui_(new Ui::LoggerWindow), model_(new LoggerModel(this))
{
#ifdef __DEBUG__
    new ModelTest(model_, this);
#endif
    gui_->setupUi(this);
    gui_->devices_->setModel(model_);
    gui_->devices_->setItemDelegateForColumn(1, new PriorityWidget(this));
    model_->initialize();
}
