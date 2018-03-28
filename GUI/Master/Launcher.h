#ifndef SIDECAR_GUI_MASTER_LAUNCHER_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_LAUNCHER_H

#include "QtCore/QSet"
#include "QtCore/QString"
#include "QtCore/QThread"
#include "QtGui/QProgressDialog"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Configuration {
class Loader;
}
namespace GUI {
namespace Master {

class ConfigurationInfo;

/** Starts the 'runner' processes associated with a given configuration file.
 */
class Launcher : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    static Logger::Log& Log();

    /** Constructor. Creates a QProgressDialog to show the launching progress and a LauncherThread to do the
        launching.

        \param parent QWidget to use as the parent for auto destruction and for
        the QProgressDialog dialog window.

        \param hosts set of host names to connect to for cleaning
    */
    Launcher(QWidget* parent, const Configuration::Loader& loader);

    /** Start the LauncherThread thread.
     */
    void start();

signals:

    /** Notification sent out after the LauncherThread finishes starting the last runner process and exits.
     */
    void finished(bool wasCanceled);

private slots:

    /** Signal handler invoked when the user clicks on the 'Cancel' button in the QProgressDialog window. Simply
        invokes the finishedAll() method.
    */
    void canceled();

private:
    void timerEvent(QTimerEvent* event);

    const Configuration::Loader& loader_;
    QProgressDialog* dialog_; ///< Dialog that shows the launching progress
    size_t index_;
    int timerId_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
