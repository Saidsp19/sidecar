#include "QtGui/QKeyEvent"

#include "ChannelConnectionView.h"

using namespace SideCar::GUI::HealthAndStatus;

void
ChannelConnectionView::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
    case Qt::Key_Clear:
        event->accept();
        if (event->modifiers() == 0)
            emit clearDrops();
        else
            emit clearAll();
        break;
    default: Super::keyPressEvent(event); break;
    }
}
