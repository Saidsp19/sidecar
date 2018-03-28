#include "QtCore/QSettings"
#include "QtGui/QAction"

#include "KeySetting.h"

using namespace SideCar::GUI;

void
KeySetting::Load(KeySetting* begin, size_t sizeOfArray, QSettings& group)
{
    KeySetting* end = begin + sizeOfArray / sizeof(KeySetting);
    while (begin < end) {
        begin->load(group);
        ++begin;
    }
}

QAction*
KeySetting::makeAction(QObject* owner, QObject* actor, const char* slot) const
{
    QAction* action = new QAction(owner);
    action->setText(getMenuText());
    action->setShortcut(getKeySequuence());
    action->setCheckable(isToggle());
    if (actor && slot) { QObject::connect(action, SIGNAL(triggered()), actor, slot); }
    return action;
}

void
KeySetting::load(QSettings& group)
{
    keySeq_ = group.value(settingName_, QVariant::fromValue(keySeq_)).value<KeySeq>();
}

void
KeySetting::normalize()
{
    int key = getKeySequuence()[0] & ~Qt::MODIFIER_MASK;
    int mods = getKeySequuence()[0] & Qt::MODIFIER_MASK;
    if (mods & Qt::SHIFT) {
        switch (key) {
        case Qt::Key_Comma: key = Qt::Key_Less; break;
        case Qt::Key_Period: key = Qt::Key_Greater; break;
        case Qt::Key_Semicolon: key = Qt::Key_Colon; break;
        case Qt::Key_Slash: key = Qt::Key_Question; break;
        case Qt::Key_Backslash: key = Qt::Key_Bar; break;
        case Qt::Key_BracketLeft: key = Qt::Key_BraceLeft; break;
        case Qt::Key_BracketRight: key = Qt::Key_BraceRight; break;
        }

        keySeq_ = KeySeq(key | mods);
    }
}
