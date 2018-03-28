#ifndef SIDECAR_GUI_MASTER_STATUSWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_STATUSWIDGET_H

#include "QtCore/QStringList"
#include "QtGui/QWidget"

#include "ui_StatusWidget.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Master {

class StatusWidget : public QWidget, public Ui::StatusWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    StatusWidget(QWidget* parent = 0);
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
