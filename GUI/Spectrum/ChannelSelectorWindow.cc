#include "GUI/ChannelGroup.h"

#include "ChannelSelectorWindow.h"

using namespace SideCar::GUI::Spectrum;

ChannelSelectorWindow::ChannelSelectorWindow(int shortcut) :
    ToolWindowBase("ChannelSelectorWindow", "Channel", shortcut), Ui::ChannelSelectorWindow()
{
    setupUi(this);
    setFixedSize();
}
