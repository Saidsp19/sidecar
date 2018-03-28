#include "Alert.h"

using namespace SideCar::GUI::Master;

void
Alert::ShowInfo(QWidget* parent, const QString& title, const QString& text)
{
    new Alert(parent, QMessageBox::Information, title, text);
}

void
Alert::ShowWarning(QWidget* parent, const QString& title, const QString& text)
{
    new Alert(parent, QMessageBox::Warning, title, text);
}

void
Alert::ShowCritical(QWidget* parent, const QString& title, const QString& text)
{
    new Alert(parent, QMessageBox::Critical, title, text);
}

Alert::Alert(QWidget* parent, QMessageBox::Icon icon, const QString& title, const QString& text) :
    QMessageBox(icon, title, text, QMessageBox::Ok, parent)
{
    connect(this, SIGNAL(finished(int)), this, SLOT(deleteLater()));
    setModal(true);
    show();
}
