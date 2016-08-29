#ifndef SIDECAR_GUI_BSCOPE_MAGNIFIERWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_MAGNIFIERWINDOW_H

#include "GUI/MainWindowBase.h"

namespace Logger { class Log; }
namespace Ui { class MagnifierWindow; }

namespace SideCar {
namespace GUI {

namespace BScope {

class MagnifierView;
class PPIWidget;

/** Window that hosts a magnified view of the MainWindow. Contains controls that allow the user to pan around
    and change magnification levels.
*/
class MagnifierWindow : public MainWindowBase
{
    Q_OBJECT
    using Super = MainWindowBase;
public:

    static Logger::Log& Log();

    MagnifierWindow(PPIWidget* renderer);

    void setBounds(double xMin, double yMin, double width, double height,
                   double xScale, double yScale);

    void drawFrame() const;

    bool showOutline() const;

public slots:

    void save(QSettings& settings) const;

    void restore(QSettings& settings);

private slots:

    void setShowPhantomCursor(bool state);

    void setShowCursorPosition(bool state);

private:

    void showEvent(QShowEvent* event);

    void closeEvent(QCloseEvent* event);

    Ui::MagnifierWindow* gui_;
    PPIWidget* renderer_;
    MagnifierView* view_;
    int viewWidth_;
    int viewHeight_;
    static int id_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
