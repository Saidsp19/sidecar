#ifndef SIDECAR_GUI_ZEROCONFMONITOR_BROWSERWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_ZEROCONFMONITOR_BROWSERWIDGET_H

#include "QtWidgets/QLabel"
#include "QtWidgets/QListWidget"

#include "GUI/ServiceBrowser.h"

namespace SideCar {
namespace GUI {
namespace ZeroconfMonitor {

class Browser;

class BrowserWidget : public QListWidget {
    Q_OBJECT
public:
    BrowserWidget(QWidget* parent = 0);

    void setLabel(QLabel* label);
    void start(const QString& type);

private slots:
    void foundServices(const ServiceEntryList& found);
    void resolved(ServiceEntry* service);
    void lostServices(const ServiceEntryList& lost);

private:
    void updateLabelCounts();

    ServiceBrowser* browser_;
    QLabel* label_;
    QString title_;
};

} // end namespace ZeroconfMonitor
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
