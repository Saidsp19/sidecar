#ifndef SIDECAR_GUI_CONTROLSWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_CONTROLSWINDOW_H

#include "GUI/ToolWindowBase.h"

class QLabel;
class QSlider;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class VideoSampleCountTransform;

class ControlsWindow : public ToolWindowBase {
    Q_OBJECT
    using Super = ToolWindowBase;

public:
    static Logger::Log& Log();

    ControlsWindow(int shortcut);

    void setVideoSampleCountTransform(const VideoSampleCountTransform* xfm);

    QSlider* getGainControl() const { return gain_; }

    QSlider* getCutoffMinControl() const { return cutoffMin_; }

    QSlider* getCutoffMaxControl() const { return cutoffMax_; }

private slots:

    void gainValueChanged(int value);

    void cutoffMinValueChanged(int value);

    void cutoffMaxValueChanged(int value);

    void update();

private:
    void showEvent(QShowEvent* event);

    QSlider* gain_;
    QLabel* gainValue_;

    QSlider* cutoffMin_;
    QLabel* cutoffMinMin_;
    QLabel* cutoffMinValue_;
    QLabel* cutoffMinMax_;

    QSlider* cutoffMax_;
    QLabel* cutoffMaxMin_;
    QLabel* cutoffMaxValue_;
    QLabel* cutoffMaxMax_;

    const VideoSampleCountTransform* transform_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
