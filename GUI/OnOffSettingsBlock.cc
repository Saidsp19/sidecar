#include "OnOffSettingsBlock.h"

using namespace SideCar::GUI;

OnOffSettingsBlock::OnOffSettingsBlock(BoolSetting* enabled)
    : SettingsBlock(), enabled_(enabled)
{
    connect(enabled, SIGNAL(valueChanged(bool)), this,
            SIGNAL(enabledChanged(bool)));
}

void
OnOffSettingsBlock::setToggleEnabledAction(QAction* action)
{
    enabled_->connectAction(action);
}
