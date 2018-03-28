#ifndef SIDECAR_GUI_SPECTRUM_SPECTROGRAPHWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_SPECTROGRAPHWINDOW_H

#include "QtCore/QList"
#include "QtGui/QImage"

class QScrollArea;

#include "GUI/MainWindowBase.h"

#include "App.h"
#include "ui_SpectrographWindow.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Spectrum {

class CursorWidget;
class FFTSettings;
class SpectrographWidget;
class ScaleWidget;

class SpectrographWindow : public MainWindowBase, private Ui::SpectrographWindow {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    static Logger::Log& Log();

    SpectrographWindow(int shortcut);

    SpectrographWidget* getDisplay() const { return display_; }

private slots:

    void showCursorPosition(const QString& x, const QString& y);

private slots:

    void on_actionShowMainWindow__triggered();

    void on_actionViewPause__triggered(bool state);

    void channelChanged(const QString& name);

private:
    SpectrographWidget* display_;
    CursorWidget* cursorWidget_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
