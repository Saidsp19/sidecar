#ifndef SIDECAR_GUI_CONFIGEDITOR_APP_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_APP_H

#include "QtCore/QDir"

#include "GUI/AppBase.h"

namespace SideCar {
namespace GUI {

/** Namespace for the ConfigEditor application.

    \image html ConfigEditor.png

    The ConfigEditor application provides a graphical editor for the SideCar
    XML configuration files used by the Master and Runner applications.
*/

namespace ConfigEditor {

class MainWindow;
class Settings;
class SettingsWindow;

/** Application class for the ConfigEditor application.
 */
class App : public AppBase {
    Q_OBJECT
    using Super = AppBase;

public:
    /** Obtain type-casted App singleton object.

        \return App object
    */
    static App* GetApp() { return static_cast<App*>(AppBase::GetApp()); }

    App(int& argc, char** argv);

    ~App();

    Settings& getSettings() const { return *settings_; }

    MainWindow* getMainWindow() const { return mainWindow_; }

    SettingsWindow* getSettingsWindow() const { return settingsWindow_; }

public slots:

    void applicationQuit();

private:
    void makeToolWindows();

    /** Override of AppBase method. Creates a new MainWindow object.

        \return MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);

    MainWindow* mainWindow_;
    Settings* settings_;
    SettingsWindow* settingsWindow_;
};

} // end namespace ConfigEditor
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
