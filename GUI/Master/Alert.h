#ifndef SIDECAR_GUI_MASTER_ALERT_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_ALERT_H

#include "QtGui/QMessageBox"

namespace SideCar {
namespace GUI {
namespace Master {

class Alert : public QMessageBox {
    Q_OBJECT
public:
    static void ShowInfo(QWidget* window, const QString& title, const QString& text);

    static void ShowWarning(QWidget* window, const QString& title, const QString& text);

    static void ShowCritical(QWidget* window, const QString& title, const QString& text);

private:
    Alert(QWidget* parent, QMessageBox::Icon icon, const QString& title, const QString& text);
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
