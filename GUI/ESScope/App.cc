#include "QtWidgets/QAction"
#include "QtWidgets/QMessageBox"

#include "GUI/ChannelSelectorWindow.h"
#include "GUI/ControlsWindow.h"
#include "GUI/LogUtils.h"
#include "GUI/PresetsWindow.h"

#include "App.h"
#include "Configuration.h"
#include "ConfigurationWindow.h"
#include "MainWindow.h"
#include "ViewEditor.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ESScope;

Logger::Log&
App::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("esscope.App");
    return log_;
}

App::App(int& argc, char** argv) :
    AppBase("ESScope", argc, argv), configuration_(0), channelSelectorWindow_(0), configurationWindow_(0),
    controlsWindow_(0), presetsWindow_(0), viewEditor_(0), mainWindow_(0)
{
    setVisibleWindowMenuNew(false);
    makeToolWindows();
    controlsWindow_->setVideoSampleCountTransform(configuration_->getVideoSampleCountTransform());
}

void
App::makeToolWindows()
{
    channelSelectorWindow_ = new ChannelSelectorWindow(Qt::CTRL + Qt::Key_1 + kShowChannelSelectorWindow);
    addToolWindow(kShowChannelSelectorWindow, channelSelectorWindow_);
    configurationWindow_ = new ConfigurationWindow(Qt::CTRL + Qt::Key_1 + kShowConfigurationWindow);
    addToolWindow(kShowConfigurationWindow, configurationWindow_);
    controlsWindow_ = new ControlsWindow(Qt::CTRL + Qt::Key_1 + kShowControlsWindow);
    addToolWindow(kShowControlsWindow, controlsWindow_);
    configuration_ = new Configuration(channelSelectorWindow_, configurationWindow_, controlsWindow_);
    viewEditor_ = new ViewEditor(Qt::CTRL + Qt::Key_1 + kShowViewEditor);
    addToolWindow(kShowViewEditor, viewEditor_);
    presetsWindow_ = new PresetsWindow(Qt::CTRL + Qt::Key_1 + kShowPresetsWindow, configuration_);
    addToolWindow(kShowPresetsWindow, presetsWindow_);
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    Logger::ProcLog log("makeNewMainWindow", Log());
    LOGINFO << "objectName: " << objectName << std::endl;

    if (objectName == "MagnifierWindow") { return 0; }

    return mainWindow_ = new MainWindow;
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
                                  QMessageBox::Cancel | QMessageBox::Save | QMessageBox::Discard);

        switch (button) {
        case QMessageBox::Save: saveChanges = true; break;

        case QMessageBox::Discard: break;

        case QMessageBox::Cancel:

            // Show the presets window for the user. NOTE: fall thru to the default case.
            //
            getPresetsWindow()->showAndRaise();

        default:

            // Something else. Default action is do no harm and avoid data loss by aborting the quitting
            // process.
            //
            return;
        }
    }

    getConfiguration()->saveAllPresets(saveChanges);

    Super::applicationQuit();
}
