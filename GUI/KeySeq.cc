#include "QtGui/QKeyEvent"

#include "KeySeq.h"

using namespace SideCar::GUI;

int KeySeq::kMetaTypeId_ = qRegisterMetaType<KeySeq>("SideCar::GUI::KeySeq");

KeySeq::KeySeq() : key_()
{
    ;
}

KeySeq::KeySeq(const QString& key) : key_(key)
{
    ;
}

KeySeq::KeySeq(const QKeySequence& key) : key_(key)
{
    ;
}

KeySeq::KeySeq(const QKeyEvent* event) : key_()
{
    unsigned int mods = event->modifiers();
    unsigned int keyMods = 0;
    if (mods & Qt::ControlModifier) keyMods |= Qt::CTRL;
    if (mods & Qt::AltModifier) keyMods |= Qt::ALT;
    if (mods & Qt::MetaModifier) keyMods |= Qt::META;
    if (mods & Qt::ShiftModifier) keyMods |= Qt::SHIFT;
    key_ = event->key() | keyMods;
}

QString
KeySeq::asString() const
{
#if defined(Q_OS_MAC)
    static const QChar QMAC_CTRL = QChar(0x2318);
    static const QChar QMAC_META = QChar(0x2303);
    static const QChar QMAC_ALT = QChar(0x2325);
    static const QChar QMAC_SHIFT = QChar(0x21E7);

    QString s(key_.toString());
    s.replace(QMAC_CTRL, "Ctrl+");
    s.replace(QMAC_META, "Meta+");
    s.replace(QMAC_ALT, "Alt+");
    s.replace(QMAC_SHIFT, "Shift+");
    return s;
#else
    return QString(key_);
#endif
}

bool
KeySeq::matches(const KeySeq& rhs) const
{
    for (size_t index = 0; index < key_.count(); ++index) {
        int key = key_[index] & ~Qt::UNICODE_ACCEL;
        for (size_t inner = 0; inner < rhs.key_.count(); ++inner) {
            int other = rhs.key_[inner] & ~Qt::UNICODE_ACCEL;
            if (key == other) return true;
        }
    }

    return false;
}
