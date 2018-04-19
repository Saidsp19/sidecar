#ifndef SIDECAR_GUI_MASTER_UTILSWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_UTILSWIDGET_H

#include "QtWidgets/QWidget"

#include "ui_UtilsWidget.h"

namespace SideCar {
namespace GUI {
namespace Master {

class UtilsWidget : public QWidget, public Ui::UtilsWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    UtilsWidget(QWidget* parent = 0);
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
