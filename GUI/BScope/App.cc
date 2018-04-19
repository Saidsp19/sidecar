#include "QtWidgets/QAction"
#include "QtWidgets/QMessageBox"

#include "GUI/ChannelSelectorWindow.h"
#include "GUI/ControlsWindow.h"
#include "GUI/LogUtils.h"
#include "GUI/PresetsWindow.h"

#include "App.h"
#include "Configuration.h"
#include "ConfigurationWindow.h"
#include "FramesWindow.h"
#include "History.h"
#include "HistorySettings.h"
#include "MainWindow.h"
#include "PPIWidget.h"
#include "PlayerWindow.h"
#include "TargetPlotImaging.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::BScope;

Logger::Log&
App::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.App");
    return log_;
}

App::App(int& argc, char** argv) :
    AppBase("BScope", argc, argv), configuration_(0), history_(new History(this)), channelSelectorWindow_(0),
    configurationWindow_(0), controlsWindow_(0), presetsWindow_(0), mainWindow_(0), framesWindow_(0), playerWindow_(0),
    past_(), blank_(1, 1, QImage::Format_RGB32), activeFrames_(0)
{
    setVisibleWindowMenuNew(false);

    channelSelectorWindow_ = new ChannelSelectorWindow(Qt::CTRL + Qt::Key_1 + kShowChannelSelectorWindow);
    addToolWindow(kShowChannelSelectorWindow, channelSelectorWindow_);

    configurationWindow_ = new ConfigurationWindow(Qt::CTRL + Qt::Key_1 + kShowConfigurationWindow);
    addToolWindow(kShowConfigurationWindow, configurationWindow_);

    controlsWindow_ = new ControlsWindow(Qt::CTRL + Qt::Key_1 + kShowControlsWindow);
    addToolWindow(kShowControlsWindow, controlsWindow_);

    configuration_ = new Configuration(channelSelectorWindow_, configurationWindow_, controlsWindow_, history_);

    HistorySettings* historySettings = configuration_->getHistorySettings();
    connect(historySettings, SIGNAL(frameCountChanged(int)), SLOT(frameCountChanged(int)));

    controlsWindow_->setVideoSampleCountTransform(configuration_->getVideoSampleCountTransform());

    presetsWindow_ = new PresetsWindow(Qt::CTRL + Qt::Key_1 + kShowPresetsWindow, configuration_);

    addToolWindow(kShowPresetsWindow, presetsWindow_);
}

void
App::restoreWindows()
{
    Super::restoreWindows();

    Q_ASSERT(mainWindow_);

    if (!playerWindow_) { makeAndInitializeNewMainWindow("PlayerWindow"); }

    addViewWindow(playerWindow_);

    if (!framesWindow_) { makeAndInitializeNewMainWindow("FramesWindow"); }

    addViewWindow(framesWindow_);
    frameCountChanged(configuration_->getHistorySettings()->getFrameCount());
}

void
App::addViewWindow(MainWindowBase* toolWindow)
{
    Logger::ProcLog log("addViewWindow", Log());
    LOGINFO << "objectName: " << toolWindow->objectName() << std::endl;

    connect(this, SIGNAL(imageSizeChanged(const QSize&)), toolWindow, SLOT(setNormalSize(const QSize&)));
    connect(this, SIGNAL(frameAdded(int)), toolWindow, SLOT(frameAdded(int)));
    connect(this, SIGNAL(newFrameCount(int)), toolWindow, SLOT(setFrameCount(int)));
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    Logger::ProcLog log("makeNewMainWindow", Log());
    LOGINFO << "objectName: " << objectName << std::endl;

    if (objectName == "MagnifierWindow") { return 0; }

    if (objectName == "PlayerWindow") {
        playerWindow_ = new PlayerWindow(past_, blank_, Qt::Key_F1);
        playerWindow_->setVisibleAfterRestore(false);
        return playerWindow_;
    }

    if (objectName == "FramesWindow") {
        framesWindow_ = new FramesWindow(past_, Qt::Key_F2);
        framesWindow_->setVisibleAfterRestore(false);
        return framesWindow_;
    }

    mainWindow_ = new MainWindow;
    blank_ = blank_.scaled(mainWindow_->getDisplay()->size());
    blank_.fill(0x808080);

    return mainWindow_;
}

void
App::frameCountChanged(int frameCount)
{
    if (frameCount < past_.size()) {
        do {
            past_.pop_back();
        } while (frameCount < past_.size());
    }

    while (frameCount > past_.size()) { past_.push_back(blank_); }

    if (activeFrames_ > frameCount) activeFrames_ = frameCount;

    emit newFrameCount(frameCount);
}

void
App::addFrame(const QImage& image)
{
    Logger::ProcLog log("addFrame", Log());
    LOGINFO << "past_.size: " << past_.size() << std::endl;
    past_.push_front(image);
    past_.pop_back();
    if (activeFrames_ < past_.size()) ++activeFrames_;
    emit frameAdded(activeFrames_);
}

void
App::setImageSize(const QSize& displaySize)
{
    blank_ = blank_.scaled(displaySize);
    for (int index = 0; index < past_.size(); ++index) past_[index] = past_[index].scaled(displaySize);
    emit imageSizeChanged(displaySize);
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
