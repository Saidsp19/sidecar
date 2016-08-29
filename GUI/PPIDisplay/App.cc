#include "QtGui/QMessageBox"

#include "GUI/ChannelSelectorWindow.h"
#include "GUI/LogUtils.h"
#include "GUI/PresetsWindow.h"
#include "GUI/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "ConfigurationWindow.h"
#include "ControlsWindow.h"
#include "History.h"
#include "HistorySettings.h"
#include "MainWindow.h"
#include "RangeTruthsImaging.h"
#include "TargetPlotImaging.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::PPIDisplay;

Logger::Log&
App::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ppidisplay.App");
    return log_;
}

App::App(int& argc, char** argv)
    : AppBase("PPIDisplay", argc, argv), configuration_(0),
      history_(new History(this, 5, false))
{
    setVisibleWindowMenuNew(false);
    makeToolWindows();
    configuration_ = new Configuration(channelSelectorWindow_,
                                       configurationWindow_,
                                       controlsWindow_);
    configurationWindow_->setConfiguration(configuration_);

    controlsWindow_->setVideoSampleCountTransform(
	configuration_->getVideoSampleCountTransform());

    HistorySettings* historySettings = configuration_->getHistorySettings();

    history_->setEnabled(historySettings->isEnabled());
    connect(historySettings, SIGNAL(enabledChanged(bool)), history_,
            SLOT(setEnabled(bool)));

    history_->setRetentionSize(historySettings->getRetentionSize());
    connect(historySettings, SIGNAL(retentionSizeChanged(int)), history_,
            SLOT(setRetentionSize(int)));

    TargetPlotImaging* imaging = configuration_->getExtractionsImaging();
    history_->setExtractionsLifeTime(imaging->getLifeTime());
    connect(imaging, SIGNAL(lifeTimeChanged(int)), history_,
            SLOT(setExtractionsLifeTime(int)));

    RangeTruthsImaging* rti = configuration_->getRangeTruthsImaging();
    history_->setRangeTruthsLifeTime(rti->getLifeTime());
    connect(imaging, SIGNAL(lifeTimeChanged(int)), history_,
            SLOT(setRangeTruthsLifeTime(int)));
    history_->setRangeTruthsMaxTrailLength(rti->getTrailSize());
    connect(rti, SIGNAL(trailSizeChanged(int)), history_,
            SLOT(setRangeTruthsMaxTrailLength(int)));

    imaging = configuration_->getBugPlotsImaging();
    history_->setBugPlotsLifeTime(imaging->getLifeTime());
    connect(imaging, SIGNAL(lifeTimeChanged(int)), history_,
            SLOT(setBugPlotsLifeTime(int)));

    presetsWindow_ = new PresetsWindow(
	Qt::CTRL + Qt::Key_1 + kShowPresetsWindow, configuration_);
    addToolWindow(kShowPresetsWindow, presetsWindow_);
}

void
App::makeToolWindows()
{
    channelSelectorWindow_ = new ChannelSelectorWindow(
	Qt::CTRL + Qt::Key_1 + kShowChannelSelectorWindow);
    addToolWindow(kShowChannelSelectorWindow, channelSelectorWindow_);

    configurationWindow_ = new ConfigurationWindow(
	Qt::CTRL + Qt::Key_1 + kShowConfigurationWindow);
    addToolWindow(kShowConfigurationWindow, configurationWindow_);

    controlsWindow_ = new ControlsWindow(
	Qt::CTRL + Qt::Key_1 + kShowControlsWindow, history_);
    addToolWindow(kShowControlsWindow, controlsWindow_);
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    static Logger::ProcLog log("makeNewMainWindow", Log());
    LOGINFO << "objectName: " << objectName << std::endl;

    if (objectName == "MagnifierWindow") {
	return 0;
    }

    return new MainWindow;
}

void
App::applicationQuit()
{
    bool saveChanges = false;

    if (getConfiguration()->getAnyIsDirty()) {
	QMessageBox::StandardButton button = 
	    QMessageBox::question(0, "Unsaved Settings",
                                  "<p>There are one or more presets that "
                                  "have been modified without saving. "
                                  "Quitting now will lose all changes.</p>",
                                  QMessageBox::Cancel |
                                  QMessageBox::Save |
                                  QMessageBox::Discard);

	switch (button) {

	case QMessageBox::Save:
	    saveChanges = true;
	    break;

	case QMessageBox::Discard:
	    break;

	case QMessageBox::Cancel:
	    getPresetsWindow()->showAndRaise();

	default:
	    return;
	}
    }

    getConfiguration()->saveAllPresets(saveChanges);
    Super::applicationQuit();
}
