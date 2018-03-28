#ifndef SIDECAR_GUI_PPIDISPLAY_HISTORYWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_HISTORYWIDGET_H

#include "ControlWidget.h"

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

class History;

class HistoryWidget : public ControlWidget {
    Q_OBJECT
    using Super = ControlWidget;

public:
    HistoryWidget(History* history, QToolBar* parent);

private slots:

    void historyRetainedCountChanged(int size);
    void historyCurrentViewChanged(int age);
    void historyCurrentViewAged(int age);

private:
    QString makeAgeText(int age);

    QLabel* ageValue_;
    History* history_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

#endif
