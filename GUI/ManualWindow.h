#ifndef SIDECAR_GUI_MANUALWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_MANUALWINDOW_H

#include "GUI/ToolWindowBase.h"

namespace Ui { class ManualWindow; }

namespace SideCar {
namespace GUI {

class ManualWindow : public ToolWindowBase
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:

    /** Constructor.
     */
    ManualWindow(const QString& name, const QString& manualPath);

    void windowMenuAboutToShow(QList<QAction*>& actions);
    
private:
    void showEvent(QShowEvent* event);

    Ui::ManualWindow* gui_;
    QString manualPath_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
