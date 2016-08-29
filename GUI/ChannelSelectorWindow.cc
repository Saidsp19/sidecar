#include "ChannelGroup.h"
#include "ChannelSelectorWindow.h"

#include "ui_ChannelSelectorWindow.h"

using namespace SideCar::GUI;

ChannelSelectorWindow::ChannelSelectorWindow(int shortcut)
    : ToolWindowBase("ChannelSelectorWindow", "Channels", shortcut)
{
    Ui::ChannelSelectorWindow init;
    init.setupUi(this);
    setFixedSize();
    foundVideo_ = init.foundVideo_;
    foundBinary_ = init.foundBinary_;
    foundExtractions_ = init.foundExtractions_;
    foundRangeTruths_ = init.foundRangeTruths_;
    foundBugPlots_ = init.foundBugPlots_;
}
