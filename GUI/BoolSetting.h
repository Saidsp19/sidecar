#ifndef SIDECAR_GUI_BOOLSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_BOOLSETTING_H

#include "GUI/Setting.h"

class QAction;
class QCheckBox;
class QRadioButton;

namespace SideCar {
namespace GUI {

/** Derivation of the Setting class that manages boolean values.
 */
class BoolSetting : public Setting
{
    Q_OBJECT
    using Super = Setting;
public:

    /** Constructor.

        \param mgr 

        \param name
        
        \param global 
    */
    BoolSetting(PresetManager* mgr, const QString& name, bool global = false);

    /** Constructor.

        \param mgr 

        \param name 

        \param value
        
        \param global 
    */
    BoolSetting(PresetManager* mgr, const QString& name, bool value, bool global = false);

    /** Obtain the current setting value.

        \return setting value
    */
    bool getValue() const { return value_; }

    void connectWidget(QCheckBox* widget);

    void connectWidget(QRadioButton* widget);

    void connectAction(QAction* action);

signals:


    /** Notification sent out when the held value changes.

        \param value new held value
    */
    void valueChanged(bool value);

public slots:

    /** Change the setting value. Emits the valueChanged() signal if the given value is diferent than the held
        one.

        \param value new value to use
    */
    void setValue(bool value);

    /** Toggle the setting value.
     */
    void toggle() { setValue(! getValue()); }

private slots:

    /** Notification handler invoked when the QAction object installed via setToggleAction() triggers. Invokes
	GUI::UpdateShowHideMenuAction() to update the action's tool tip.
    */
    void updateAction();

private:

    /** Override of Setting::valueUpdated() method. Records the new value and emits the valueChanged() signal.
     */
    void valueUpdated();

    bool value_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
