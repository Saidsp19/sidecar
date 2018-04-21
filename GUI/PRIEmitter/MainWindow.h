#ifndef SIDECAR_GUI_PRIEMITTER_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_PRIEMITTER_MAINWINDOW_H

#include "QtCore/QFile"

#include "GUI/MainWindowBase.h"
#include "IO/Readers.h"

#include "ui_MainWindow.h"

class QFileInfo;
class QProgressDialog;
class QTimer;

namespace SideCar {
namespace GUI {

class MessageWriter;

namespace PRIEmitter {

/** Main window of the PRIEmitter application. Can read Video PRI messages stored in a file. Publishes
    connection information to the local network, allowing other applications to subscribe to the published data
    feed. Each connection is handled by its own WriterThread object which handles the actual writing to its
    subscriber. The WriterThread objects receive PRI messages to send when MainWindow emits the writeMessage()
    signal.

    Currently, the entire file to load is read into memory. For large files, this is clearly undesirable.
*/
class MainWindow : public MainWindowBase, private Ui::MainWindow {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    /** NOTE: the entries here must match the order of the entries in the connectionType_ QComboBox widget.
     */
    enum ConnectionType { kTCP, kMulticast };

    /** Obtain the log device for MainWindow objects. Uses the name 'priemitter.MainWindow'.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.
     */
    MainWindow();

    /** Open a data file to emit.

        \param path location of the file to load
    */
    void openFile(const QString& path);

private slots:
    void updateButtons();
    void on_frequency__valueChanged(int value);
    void on_startStop__clicked();
    void on_resetRate__clicked();
    void on_rewind__clicked();
    void on_step__currentIndexChanged(int index);
    void on_loopAtEnd__toggled(bool value);
    void on_connectionType__currentIndexChanged(int index);

    void on_name__editingFinished();
    void on_address__editingFinished();

    void on_actionFileOpen__triggered();
    void on_actionRecentFilesClear__triggered();
    void on_actionEmitterStart__triggered();
    void on_actionEmitterRewind__triggered();
    void on_simAz__toggled(bool checked);

    /** Timer timeout notification. Emits the next message to the subscribers.
     */
    void emitMessage();

    /** Notification that the user has chosen an item from the recent files menu.
     */
    void openRecentFile();

    void writerSubscriberCountChanged(size_t size);

    void writerPublished(const QString& serviceName, const QString& serverAddress, uint16_t port);

    void writerFailure();

private:
    /** Change the timing interval to reflect the value in the frequency_ widget.
     */
    void updateTimer();

    /** Read in a text log file, and create an equivalent Video PRI file.

        \param fileInfo location of the file to open

        \return true if successful
    */
    bool openLogFile(const QFileInfo& fileInfo);

    /** Open and load a Video PRI file.

        \param fileInfo location of the file to open

        \return true if successful
    */
    bool openPRIFile(const QFileInfo& fileInfo);

    /** See if there is an XML configuration that corresponds to a data file. Looks for an XML file with the
        same path and file name as the data file, but with a suffix of '.xml'. If an XML file does not exist,
        then

        \param fileInfo location of the data file that is being opened.
    */
    void openConfigFile(const QFileInfo& fileInfo);

    /** Update the window title and recent files menu with the name of the file that was just loaded..

        \param path path of the file
    */
    void setCurrentFile(const QString& path);

    /** Update the recent files menu items.
     */
    void updateRecentFileActions();

    /** Obtain the name of a file, sans any path.

        \param path full path of a file

        \return file name
    */
    QString strippedName(const QString& path);

    void makeWriter();

    void removeWriter();

    void rewind();
    void loadDone(const QString& path);

    QFile* file_;
    IO::FileReader::Ref reader_;
    QProgressDialog* progressDialog_;

    MessageWriter* writer_;
    QString lastName_;
    QString lastAddress_;
    QTimer* timer_;
    QString lastFile_;
    enum { kMaxRecentFiles = 10 };
    QAction* recentFiles_[kMaxRecentFiles];

    double beamWidthValue_;
    double shaftDelta_;
    double simulatedShaft_;
    bool emitting_;
};

} // namespace PRIEmitter
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
