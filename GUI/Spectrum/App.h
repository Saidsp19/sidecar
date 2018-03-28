#ifndef SIDECAR_GUI_SPECTRUM_APP_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_APP_H

#include "GUI/AppBase.h"
#include "GUI/MessageList.h"
#include "Messages/PRIMessage.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class PresetsWindow;

/** Namespace for the Spectrum application.

    \image html Spectrum.png

    The Spectrum application is a spectrum analyser for 331 SideCar signals.
*/
namespace Spectrum {

class Configuration;
class ConfigurationWindow;
class MainWindow;
class SpectrographWindow;
class SpectrumWidget;
class ViewEditor;
class WeightWindow;
class WorkerThread;
class WorkRequest;

/** Application class definition. Manages tool windows shared by all MainWindow objects. Also creates a History
    object that other classes use to obtain Video data for display.
*/
class App : public AppBase {
    Q_OBJECT
    using Super = AppBase;

public:
    /** Menu action enumeration for tool window show/hide actions.
     */
    enum ToolsMenuAction { kShowConfigurationWindow, kShowViewEditor, kShowPresetsWindow, kNumToolsMenuActions };

    /** Log device to use for App log messages.

        \return Log device
    */
    static Logger::Log& Log();

    /** Obtain the App singleton.

        \return App object
    */
    static App* GetApp() { return dynamic_cast<App*>(qApp); }

    /** Constructor.

        \param argc argument count from the command line

        \param argv argument values from the command line
    */
    App(int& argc, char** argv);

    QAction* getToolsMenuAction(ToolsMenuAction index) { return Super::getToolsMenuAction(index); }

    Configuration* getConfiguration() const { return configuration_; }

    WeightWindow* getWeightWindow() const { return weightWindow_; }

    /** Obtain the configuration tool window

        \return ConfigurationWindow window
    */
    ViewEditor* getViewEditor() const { return viewEditor_; }

    MainWindow* getMainWindow() const { return mainWindow_; }

    SpectrographWindow* getSpectrographWindow();

    PresetsWindow* getPresetsWindow() const { return presetsWindow_; }

public slots:

    void processVideo(const MessageList& data);

private slots:

    void fftSizeChanged(int);

    void threadFinished();

    void shutdownThreads();

    void setWorkerThreadCount(int count);

    void applicationQuit();

private:
    /** Override of AppBase method. Create a MainWindow object after the applicadtion has finished starting up.

        \return new MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);

    /** Create the tool windows.
     */
    void makeToolWindows();

    Configuration* configuration_;
    ConfigurationWindow* configurationWindow_;
    ViewEditor* viewEditor_;
    MainWindow* mainWindow_;
    SpectrographWindow* spectrographWindow_;
    WeightWindow* weightWindow_;
    QList<WorkerThread*> workerThreads_;
    QList<WorkerThread*> idleWorkerThreads_;
    SpectrumWidget* display_;
    PresetsWindow* presetsWindow_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
