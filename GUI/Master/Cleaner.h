#ifndef SIDECAR_GUI_MASTER_CLEANER_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_CLEANER_H

#include "QtCore/QSet"
#include "QtCore/QString"
#include "QtCore/QThread"
#include "QtWidgets/QProgressDialog"

namespace SideCar {
namespace GUI {
namespace Master {

/** Worker thread used by the Cleaner class to perform the cleaning actions. Invokes a remote 'killall' command
    on each host given in the constructor that looks for and terminates 'runner' and 'tail' processes running
    under the user's login ID.

    Emits the finishedHost() signal after each 'killall' command completes, and
    a finished() signal when it has finished processing all hosts.
*/
class CleanerThread : public QThread {
    Q_OBJECT
    using Super = QThread;

public:
    /** Constructor

        \param hosts names of the hosts to connect to and clean
    */
    CleanerThread(const QSet<QString>& hosts);

    /** Stop an active thread the reap it.
     */
    void stop();

signals:

    /** Notification sent out after cleaning a host.

        \param host the name of the cleaned host
    */
    void finishedHost(QString host);

private:
    /** Implementation of QThread::run() method. Loops over the set of host names given in the constructor, and
        executes a 'killall' command on each to terminate active 'runner' and 'tail' processes.
    */
    void run();

    QSet<QString> hosts_; ///< Names of hosts to processes
    volatile bool stop_;  ///< Flag to signal running thread to stop
};

/** Terminates active 'runner' and 'tail' processes on a set of hosts that are running under the current user's
    ID. In normal operation, the SideCar system properly starts and stops remote processes. However, in abnormal
    circumstances, a process may fail to terminate properly, in which case a Cleaner instance will do the job,
    indiscriminately and with gusto.
*/
class Cleaner : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    /** Constructor. Creates a QProgressDialog to show the cleaning progress and a CleanerThread to do the
        cleaning. Note that it does not start the CleanerThread; call the start() method when ready to do so.

        \param parent QWidget to use as the parent for auto destruction and for
        the QProgressDialog dialog window.

        \param hosts set of host names to connect to for cleaning
    */
    Cleaner(QWidget* parent, const QSet<QString>& hosts);

    /** Destructor. Stop and destroy the cleaning thread if it exists. Close and destroy the QProgressDialog if
        it still exists.
    */
    ~Cleaner();

    /** Start the CleanerThread thread.
     */
    void start();

signals:

    /** Notification sent out after the CleanerThread finishes processing the last host and exits.
     */
    void finished();

private slots:

    /** Signal handler that is invoked when the CleanerThread instance finishes processing a host.

        \param host the name of the host that was processed.
    */
    void finishedHost(QString host);

    /** Signal handler that is invoked when the CleanerThread thread exits its CleanerThread::run() method.
        Reaps the thread and deletes it, and tears down the progress dialog window and deletes it. Emits the
        finished() signal.
    */
    void finishedAll();

    /** Signal handler invoked when the user clicks on the 'Cancel' button in the QProgressDialog window. Simply
        invokes the finishedAll() method.
    */
    void canceled();

private:
    CleanerThread* thread_;   ///< Worker thread that does the cleaning
    QProgressDialog* dialog_; ///< Dialog that shows the cleaning progress
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
