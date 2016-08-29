#include "GUI/CLUT.h"

#include "ConfigurationWindow.h"

using namespace SideCar::GUI::Spectrum;

ConfigurationWindow::ConfigurationWindow(int shortcut)
    : ToolWindowBase("ConfigurationWindow", "Settings", shortcut),
      Ui::ConfigurationWindow()
{
    setupUi(this);
    setFixedSize();
    CLUT::AddTypeNames(spectrographColorMap_);
}
