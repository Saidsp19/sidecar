#ifndef SIDECAR_GUI_ONOFFSETTINGSBLOCK_H // -*- C++ -*-
#define SIDECAR_GUI_ONOFFSETTINGSBLOCK_H

#include "GUI/BoolSetting.h"

#include "GUI/SettingsBlock.h"

namespace SideCar {
namespace GUI {

/** Derivation of SettingsBlock that has a BoolSetting object which acts as an 'enabled' setting. Defines an
    additional signal, enabledChanged(), which is emitted whenever the enabled state changes. NOTE: enabled
    state changes do not emit the SettingsBlock::settingsChanged() signal.
*/
class OnOffSettingsBlock : public SettingsBlock
{
    Q_OBJECT
    using Super = SettingsBlock;
public:

    /** Constructor.

        \param enabled setting that controls this object's enabled state
    */
    OnOffSettingsBlock(BoolSetting* enabled);

    /** Determine if the settings block is enabled

        \return true if so
    */
    bool isEnabled() const { return enabled_->getValue(); }

    /** Toggle the enabled state of this settings block.
     */
    void toggleEnabled() { enabled_->toggle(); }

    /** Install a QAction object to control the on/off state of the settings block.

        \param action the QAction to use
    */
    void setToggleEnabledAction(QAction* action);

    void connectWidget(QCheckBox* widget)
	{ enabled_->connectWidget(widget); }

signals:

    /** Notification sent out when the enabled state has changed.
     */
    void enabledChanged(bool state);

private:
    BoolSetting* enabled_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
