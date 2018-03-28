#include <iostream>

#include "QtGui/QKeyEvent"

#include "AddressLineEdit.h"

using namespace SideCar::GUI::RangeTruthEmitter;

#ifdef darwin
#define MOD Qt::MetaModifier
#else
#define MOD Qt::ControlModifier
#endif

void
AddressLineEdit::keyPressEvent(QKeyEvent* event)
{
    static const Qt::KeyboardModifier kControlMod = MOD;
    switch (event->key()) {
    case Qt::Key_A:
        if (event->modifiers() == kControlMod) {
            home(false);
            return;
        }
        break;
    case Qt::Key_E:
        if (event->modifiers() == kControlMod) {
            end(false);
            return;
        }
        break;
    case Qt::Key_K:
        if (event->modifiers() == kControlMod) {
            end(true);
            del();
            return;
        }
        break;
    }

    Super::keyPressEvent(event);
}
