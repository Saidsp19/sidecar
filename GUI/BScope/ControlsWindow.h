#ifndef SIDECAR_GUI_BSCOPE_CONTROLSWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_CONTROLSWINDOW_H

#include "GUI/ToolWindowBase.h"

#include "ui_ControlsWindow.h"

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {

class VideoSampleCountTransform;

namespace BScope {

class Configuration;
class ConfigurationWindow;

class ControlsWindow : public ToolWindowBase, public Ui::ControlsWindow {
    Q_OBJECT
    using Super = ToolWindowBase;

public:
    static Logger::Log& Log();

    ControlsWindow(int shortcut);

    void setConfiguration(const Configuration* configuration);

private slots:

    void on_gain__valueChanged(int value);
    void on_cutoffMin__valueChanged(int value);
    void on_cutoffMax__valueChanged(int value);
    void update();

private:
    VideoSampleCountTransform* transform_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
