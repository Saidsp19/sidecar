#ifndef SIDECAR_GUI_PPIDISPLAY_CONTROLSWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_CONTROLSWINDOW_H

#include "GUI/ControlsWindow.h"

class QLabel;

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

class History;

class ControlsWindow : public GUI::ControlsWindow {
    Q_OBJECT
    using Super = GUI::ControlsWindow;

public:
    static Logger::Log& Log();

    ControlsWindow(int shortcut, History* history);

    QSlider* getAgeControl() const { return age_; }

private slots:

    void setAgeText(int age);

    void historyRetainedCountChanged(int count);

    void historyCurrentViewChanged(int age);

    void historyCurrentViewAged(int age);

private:
    QSlider* age_;
    QLabel* ageValue_;
    History* history_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
