#ifndef SIDECAR_GUI_PPIDISPLAY_SCREENCAPTUREWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_SCREENCAPTUREWINDOW_H

#include "QtCore/QTime"
#include "QtCore/QTimer"

#include "GUI/ToolWindowBase.h"

#include "ui_ScreenCaptureWindow.h"

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

class ImageWriter;
class PPIWidget;

class ScreenCaptureWindow : public ToolWindowBase, private Ui::ScreenCaptureWindow {
    Q_OBJECT
public:
    ScreenCaptureWindow(QAction* action);

    void setDisplay(PPIWidget* display) { display_ = display; }

private slots:
    void on_startStop__clicked();
    void on_frameRate__valueChanged(int value);
    void on_duration__valueChanged(int value);
    void capture();
    void imageWriterDone();
    void updateStatusBar();

private:
    void stop();

    QTimer timer_;
    ImageWriter* writer_;
    PPIWidget* display_;
    QTime elapsed_;
    QString lastCaptureDirectory_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

#endif
