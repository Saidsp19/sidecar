#include "QtCore/QFile"

#include "ManualWindow.h"
#include "ui_ManualWindow.h"

using namespace SideCar::GUI;

ManualWindow::ManualWindow(const QString& appName, const QString& manualPath) :
    Super(appName, appName + " Manual"), gui_(new Ui::ManualWindow), manualPath_(manualPath)
{
    gui_->setupUi(this);
    setObjectName("ManualWindow");
    setWindowTitle(appName + " Manual");
    gui_->contents_->setReadOnly(true);
}

void
ManualWindow::showEvent(QShowEvent* event)
{
    // Load the manual contents. Done here to facilitate editing, but this should be done just once and cached.
    //
    QFile file(manualPath_);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        gui_->contents_->setHtml(QString(file.readAll()));
    } else {
        gui_->contents_->setPlainText("The manual for this application is not "
                                      "installed.");
    }

    Super::showEvent(event);
}
