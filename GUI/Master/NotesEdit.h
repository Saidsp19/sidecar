#ifndef SIDECAR_GUI_NOTESEDIT_H // -*- C++ -*-
#define SIDECAR_GUI_NOTESEDIT_H

#include "QtWidgets/QTextEdit"

namespace SideCar {
namespace GUI {
namespace Master {

/** Derivation of a QTextEdit that containes a context-menu with actions to insert timestamps.
 */
class NotesEdit : public QTextEdit {
    Q_OBJECT
public:
    NotesEdit(QWidget* parent) : QTextEdit(parent), contextMenu_(0) {}

    ~NotesEdit();

    void setNotesWindowActions(QAction* insertNow, QAction* insertElapsed);

private:
    void contextMenuEvent(QContextMenuEvent* event);

    QMenu* contextMenu_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
