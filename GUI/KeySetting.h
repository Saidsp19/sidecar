#ifndef SIDECAR_GUI_KEYSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_KEYSETTING_H

#include "KeySeq.h"

class QAction;
class QSettings;

namespace SideCar {
namespace GUI {

class KeySetting {
public:
    static void Load(KeySetting* begin, size_t sizeOfArray, QSettings& group);

    KeySetting(const char* settingName, const char* menuText, int defaultKey, bool isToggle)
	: settingName_(settingName), menuText_(menuText), keySeq_(defaultKey), isToggle_(isToggle)
        { normalize(); }

    KeySetting(const char* menuText, int defaultKey, bool isToggle)
	: settingName_(menuText), menuText_(menuText), keySeq_(defaultKey), isToggle_(isToggle)
	{ settingName_.remove(' '); normalize(); }

    KeySetting(const char* menuText, int defaultKey)
	: settingName_(menuText), menuText_(menuText), keySeq_(defaultKey), isToggle_(false)
	{ settingName_.remove(' '); normalize(); }

    const QString& getSettingName() const { return settingName_; }
    
    const QString& getMenuText() const { return menuText_; }

    const QKeySequence& getKeySequuence() const { return keySeq_.getKeySequence(); }

    bool isToggle() const { return isToggle_; }

    QAction* makeAction(QObject* owner) const { return makeAction(owner, 0, 0); }

    QAction* makeAction(QObject* owner, QObject* actor, const char* slot) const;

    void load(QSettings& group);

private:
    void normalize();

    QString settingName_;
    QString menuText_;
    KeySeq keySeq_;
    bool isToggle_;
};

}
}

/** \file
 */

#endif
