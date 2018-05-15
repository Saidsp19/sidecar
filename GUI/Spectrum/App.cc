#include "QtCore/QSettings"
#include "QtGui/QIcon"
#include "QtWidgets/QMessageBox"

#include "GUI/LogUtils.h"
#include "GUI/PresetsWindow.h"
#include "GUI/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "ConfigurationWindow.h"
#include "FFTSettings.h"
#include "MainWindow.h"
#include "Settings.h"
#include "SpectrographWidget.h"
#include "SpectrographWindow.h"
#include "SpectrumWidget.h"
#include "ViewEditor.h"
#include "WeightWindow.h"
#include "WorkRequest.h"
#include "WorkerThread.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Spectrum;

Logger::Log&
App::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("spectrum.App");
    return log_;
}

App::App(int& argc, char** argv) :
    AppBase("Spectrum", argc, argv), configurationWindow_(nullptr), viewEditor_(nullptr), mainWindow_(nullptr),
    spectrographWindow_(nullptr), weightWindow_(nullptr), workerThreads_(), idleWorkerThreads_(),
    display_(nullptr)
{
    static Logger::ProcLog log("App", Log());
    LOGINFO << std::endl;
    setVisibleWindowMenuNew(false);
    makeToolWindows();
    connect(this, &App::aboutToQuit, this, &App::shutdownThreads);
}

void
App::makeToolWindows()
{
    configurationWindow_ = new ConfigurationWindow(Qt::CTRL + Qt::Key_1 + kShowConfigurationWindow);
    addToolWindow(kShowConfigurationWindow, configurationWindow_);

    configuration_ = new Configuration(configurationWindow_);

    viewEditor_ = new ViewEditor(Qt::CTRL + Qt::Key_1 + kShowViewEditor);
    addToolWindow(kShowViewEditor, viewEditor_);

    presetsWindow_ = new PresetsWindow(Qt::CTRL + Qt::Key_1 + kShowPresetsWindow, configuration_);
    addToolWindow(kShowPresetsWindow, presetsWindow_);

    FFTSettings* fftSettings = configuration_->getFFTSettings();
    weightWindow_ = new WeightWindow(fftSettings);
    setWorkerThreadCount(fftSettings->getWorkerThreadCount());

    connect(fftSettings, &FFTSettings::fftSizeChanged, this, &App::fftSizeChanged);
    connect(fftSettings, &FFTSettings::workerThreadCountChanged, this, &App::setWorkerThreadCount);

    Settings* settings = configuration_->getSettings();
    connect(settings->getInputChannel(), &ChannelSetting::incoming, this, &App::processVideo);
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    static Logger::ProcLog log("makeNewMainWindow", Log());
    LOGERROR << "objectName: " << objectName << std::endl;

    if (objectName == "SpectrographWindow") return getSpectrographWindow();

    Q_ASSERT(display_ == 0);
    mainWindow_ = new MainWindow;
    display_ = mainWindow_->getDisplay();

    if (spectrographWindow_) {
        connect(display_, &SpectrumWidget::binsUpdated, spectrographWindow_->getDisplay(),
                &SpectrographWidget::processBins);
    }

    viewEditor_->setSpectrumWidget(display_);

    return mainWindow_;
}

void
App::processVideo(const MessageList& data)
{
    static Logger::ProcLog log("processVideo", Log());
    LOGINFO << std::endl;

    int available = idleWorkerThreads_.size();
    int size = data.size();

    int index = size - available;
    if (index < 0) index = 0;

    for (; index < size; ++index) {
        Q_ASSERT(!idleWorkerThreads_.empty());

        Messages::Video::Ref msg = boost::dynamic_pointer_cast<Messages::Video>(data[index]);

        // Fetch the next available WorkerThread, add data to it, and if it is ready for processing, then remove
        // it from the set of available threads and start processing.
        //
        WorkerThread* workerThread = idleWorkerThreads_.back();
        if (workerThread->getWorkRequest()->addData(msg)) {
            LOGDEBUG << "processing" << std::endl;
            idleWorkerThreads_.pop_back();
            workerThread->submit();
        }
    }
}

void
App::threadFinished()
{
    static Logger::ProcLog log("threadFinished", Log());
    LOGINFO << std::endl;

    WorkerThread* workerThread = qobject_cast<WorkerThread*>(sender());

    // Make sure that the thread still exists. It could have been removed by setWorkerThreadCount().
    //
    int index = workerThreads_.indexOf(workerThread);
    if (index != -1) {
        WorkRequest* workRequest = workerThread->getWorkRequest();
        if (workRequest) {
            display_->setData(workRequest->getLastMessage(), workRequest->getOutput());
            idleWorkerThreads_.push_back(workerThread);
        }
    }
}

void
App::shutdownThreads()
{
    Logger::ProcLog log("shutdownThreads", Log());
    LOGINFO << std::endl;
    setWorkerThreadCount(0);
}

void
App::fftSizeChanged(int fftSize)
{
    static Logger::ProcLog log("fftSizeChanged", Log());
    LOGINFO << "fftSize: " << fftSize << std::endl;
    if (size_t(fftSize) != weightWindow_->getSize()) {
        weightWindow_->setSize(fftSize);
        int lastThreadCount = workerThreads_.size();
        setWorkerThreadCount(0);
        setWorkerThreadCount(lastThreadCount);
    }
}

void
App::setWorkerThreadCount(int value)
{
    static Logger::ProcLog log("setWorkerThreadCount", Log());
    LOGINFO << "value: " << value << " current: " << workerThreads_.size() << std::endl;

    LOGINFO << "workerThreads size: " << workerThreads_.size()
            << " idleWorkerThreads size: " << idleWorkerThreads_.size() << std::endl;

    if (value == workerThreads_.size()) return;

    while (value < workerThreads_.size()) {
        // First, remove the thread from the workerThreads list. The threadFinished() method checks for
        // membership in this before sending the FFT data to the display.
        //
        WorkerThread* workerThread = workerThreads_.takeLast();
        LOGDEBUG << "stopping thread" << std::endl;
        workerThread->stop();

        // Call sendPostedEvents() to flush out any pending calls to threadFinished() since a thread may have
        // submitted an asynchronous call to that method while we have been in this one.
        //
        QCoreApplication::sendPostedEvents(this, 0);
        LOGDEBUG << "workerThreads size: " << workerThreads_.size()
                 << " idleWorkerThreads_ size: " << idleWorkerThreads_.size() << std::endl;

        int index = idleWorkerThreads_.indexOf(workerThread);
        if (index != -1) idleWorkerThreads_.removeAt(index);

        // Don't do an immediate delete, since there may be outstanding asynchronous submitRequest() calls.
        //
        workerThread->deleteLater();
    }

    if (value == 0) return;

    while (value > workerThreads_.size()) {
        WorkRequest* workRequest = workerThreads_.empty() ? new WorkRequest(*weightWindow_)
                                                          : new WorkRequest(*workerThreads_[0]->getWorkRequest());
        WorkerThread* workerThread = new WorkerThread(workRequest);
        workerThreads_.append(workerThread);
        connect(workerThread, &WorkerThread::finished, this, &App::threadFinished, Qt::QueuedConnection);
        idleWorkerThreads_.append(workerThread);
    }
}

SpectrographWindow*
App::getSpectrographWindow()
{
    if (!spectrographWindow_) {
        spectrographWindow_ = new SpectrographWindow(Qt::Key_F3);
        if (display_) {
            connect(display_, &SpectrumWidget::binsUpdated, spectrographWindow_->getDisplay(),
                    &SpectrographWidget::processBins);
        }
    }

    return spectrographWindow_;
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

        case QMessageBox::Cancel: getPresetsWindow()->showAndRaise();

        default: return;
        }
    }

    getConfiguration()->saveAllPresets(saveChanges);
    Super::applicationQuit();
}
