#ifndef SIDECAR_GUI_MASTER_LOGALERTSWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_LOGALERTSWINDOW_H

#include "GUI/MainWindowBase.h"

namespace Ui {
class LogAlertsWindow;
}

namespace SideCar {
namespace GUI {
namespace Master {

/** Window that shows SideCar Logger::Log messages of level Logger::Priority::kWarning or greater.
 */
class LogAlertsWindow : public MainWindowBase {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    LogAlertsWindow();

public slots:

    void addAlerts(const QStringList& entry);

private:
    Ui::LogAlertsWindow* gui_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
