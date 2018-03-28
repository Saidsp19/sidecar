#include "ConfigurationWindow.h"
#include "App.h"
#include "ChannelPlotSettings.h"

using namespace SideCar::GUI::HealthAndStatus;

ConfigurationWindow::ConfigurationWindow(int shortcut) :
    ToolWindowBase("ConfigurationWindow", "Settings", shortcut), Ui::ConfigurationWindow(),
    defaults_(new ChannelPlotSettings(App::GetApp()->getPresetManager(), this, "Defaults")), connected_()
{
    setupUi(this);
    setFixedSize();
    defaults_->connectToEditor(this);
    updateButtons();
}

void
ConfigurationWindow::addChannelPlotSettings(ChannelPlotSettings* settings)
{
    if (!connected_.contains(settings)) {
        connected_.append(settings);
        settings->connectToEditor(this);
        if (connected_.size() == 1) { updateFromSettings(settings); }
    }

    updateButtons();
}

void
ConfigurationWindow::updateFromSettings(ChannelPlotSettings* settings)
{
    samplesColor_->setColor(settings->getSamplesColor());
    messageDecimation_->setValue(settings->getMessageDecimation());
    sampleStartIndex_->setValue(settings->getSampleStartIndex());
    sampleCount_->setValue(settings->getSampleCount());
    runningMedianWindowSize_->setValue(settings->getRunningMedianWindowSize());
    algorithm_->setCurrentIndex(settings->getAlgorithm());
    expectedVariance_->setValue(settings->getExpectedVariance());
    expected_->setValue(settings->getExpected());
}

void
ConfigurationWindow::removeChannelPlotSettings(ChannelPlotSettings* settings)
{
    settings->disconnectFromEditor(this);
    connected_.removeAll(settings);
    if (connected_.size() == 1) {
        updateFromSettings(connected_[0]);
    } else if (connected_.size() == 0) {
        updateFromSettings(defaults_);
    }
    updateButtons();
}

void
ConfigurationWindow::on_updateExpected__clicked()
{
    if (!connected_.empty()) { expected_->setValue(connected_[0]->getCurrentValue()); }
}

void
ConfigurationWindow::on_updateAll__clicked()
{
#if 0
  if (connected_.size() > 1) {
    for (int index = 1; index < connected_.size(); ++index)
      connected_[index]->copyFrom(connected_[0]);
  }
#endif
}

void
ConfigurationWindow::updateButtons()
{
    int size = connected_.size();
    if (size == 0) {
        setWindowTitle("Default Settings");
        updateExpected_->hide();
        updateAll_->hide();
    } else if (size == 1) {
        setWindowTitle(connected_[0]->getName() + " Settings");
        updateExpected_->show();
        updateAll_->hide();
    } else {
        setWindowTitle("Multiple Settings");
        updateExpected_->show();
        updateAll_->show();
    }
}
