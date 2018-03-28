#include "QtGui/QAction"
#include "QtGui/QContextMenuEvent"
#include "QtGui/QMenu"

#include "ToolBar.h"

using namespace SideCar::GUI;

ToolBar::ToolBar(const QString& title, QMenu* toolBarMenu, QWidget* parent) :
    Super(title, parent), contextMenu_(new QMenu(this))
{
    contextMenu_->addAction("Hide", toggleViewAction(), SLOT(trigger()));
    contextMenu_->addSeparator();
    contextMenu_->addMenu(toolBarMenu);
}

void
ToolBar::toggleVisibility()
{
    toggleViewAction()->activate(QAction::Trigger);
}

void
ToolBar::contextMenuEvent(QContextMenuEvent* event)
{
    event->accept();
    contextMenu_->popup(event->globalPos(), actions().first());
}
