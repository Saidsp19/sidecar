#ifndef SIDECAR_GUI_SETTINGSBLOCK_H // -*- C++ -*-
#define SIDECAR_GUI_SETTINGSBLOCK_H

#include "QtCore/QObject"

namespace SideCar {
namespace GUI {

class Setting;

/** Base class for a collection of Setting objects that conceptually belong together as a unit. The
    SettingsBlock::add() method allows one to add a Setting object to it. SettingsBlock objects do not keep
    around pointers to the added Setting objects, rather the SettingsBlock object will receive notification via
    its postSettingChanged() slot when a Setting object changes its held value. The postSettingChanged() method
    will register a one-shot QTimer object that will invoke the emitSettingChanged() slot as soon as the
    application's event loop is reentered. Only one QTime registration will take place, regardless of how many
    setting changes there are before the application's event loop is entered.
*/
class SettingsBlock : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    /** Constructor.
     */
    SettingsBlock(QObject* parent = 0)
	: Super(parent), pendingNotification_(false) {}

signals:

    /** Notification sent out when a registered value has changed.
     */
    void settingChanged();

protected slots:

    /** Notification handler invoked when a Setting object that was registered with one of the add() methods
	emits its valueChanged() signal. Sets up a QTimer to call emitSettingChanged() when the application
	event loop had nothing more to do. The emitSettingChanged() resets the one-shot boolean
	pendingNotification_ attribute and then emits the settingChanged() signal.
	
    */
    virtual void postSettingChanged();

    /** Notification handler invoked when the QTimer object registered by the postSettingChanged() method fires.
	Resets the pendingNotification_ attribute and emits the settingChanged() signal.
    */
    virtual void emitSettingChanged();

protected:
    
    /** Relate a Setting object to this SettingsBlock object.

        \param setting the object to connect
    */
    void add(Setting* setting);

private:
    bool pendingNotification_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
