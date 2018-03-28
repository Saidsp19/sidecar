#ifndef SIDECAR_GUI_TOOLBAR_H // -*- C++ -*-
#define SIDECAR_GUI_TOOLBAR_H

#include "QtGui/QToolBar"

class QMenu;

namespace SideCar {
namespace GUI {

/** Simple derivation of the QToolBar widget that can show a context menu containing a QAction to hide itself
    when selected.
*/
class ToolBar : public QToolBar {
    Q_OBJECT
    using Super = QToolBar;

public:
    /** Constructor.

        \param title

        \param parent
    */
    ToolBar(const QString& title, QMenu* toolBarMenu, QWidget* parent = 0);

public slots:

    void toggleVisibility();

private:
    /** Override of QWidget method. Hides the toolbar by activating QAction object returned by
        QToolBar::toggleViewAction() method.

        \param event description of the event that occured
    */
    void contextMenuEvent(QContextMenuEvent* event);

    QMenu* contextMenu_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
