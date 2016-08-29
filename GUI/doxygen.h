#ifndef SIDECAR_GUI_DOXYGEN_H // -*- C++ -*-
#define SIDECAR_GUI_DOXYGEN_H

/** \page gui "GUI Framework"

    Applications that need a user interface use the <a href="www.trolltech.com">Qt library</a> from Trolltech.
    The Qt library contains a mature collection of widgets one can use to quickly build GUI applications. All of
    the SideCar classes related to GUI development are found under the GUI directory, and the classes exist
    within the SideCar::GUI namespace. To attempt some uniformity across all applications, there is are
    SideCar::GUI::AppBase, SideCar::GUI::MainWindowBase, and SideCar::GUI::ToolWindowBase classes that define
    common behavior for their respective derived classes.

    All of the applications utilize Qt's QSettings class to record and retrieve application settings. Things
    such as window position and size, channel connections, and pen colors are all saved to disk as the user
    works with the application. When the user restarts an application, anything that saved a setting value to
    disk during the previous session will restore itself with the same setting values. As a result, window
    positions and channel selections remain the same across application invocations. Documentation for the GUI
    directory.
*/

namespace SideCar {

/** Namespace for the GUI applications. Each application has its own namespace with the same name as the
    application. The application namespaces are listed below.
*/
namespace GUI {}

} // end namespace SideCar

#endif
