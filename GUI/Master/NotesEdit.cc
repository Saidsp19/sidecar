#include "QtGui/QContextMenuEvent"
#include "QtWidgets/QAction"
#include "QtWidgets/QMenu"

#include "NotesEdit.h"
#include "NotesWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

NotesEdit::~NotesEdit()
{
    delete contextMenu_;
}

void
NotesEdit::setNotesWindowActions(QAction* insertNow, QAction* insertElapsed)
{
    if (contextMenu_) delete contextMenu_;
    contextMenu_ = createStandardContextMenu();
    QAction* before = contextMenu_->actions()[0];
    contextMenu_->insertAction(before, insertNow);
    contextMenu_->insertAction(before, insertElapsed);
    QAction* separator = new QAction(contextMenu_);
    separator->setSeparator(true);
    contextMenu_->insertAction(before, separator);
}

void
NotesEdit::contextMenuEvent(QContextMenuEvent* event)
{
    contextMenu_->exec(event->globalPos());
}
