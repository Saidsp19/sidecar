#ifndef SIDECAR_GUI_MAINWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_MAINWINDOW_H

#include <vector>

#include "GUI/MainWindowBase.h"
#include "GUI/Writers.h"
#include "Messages/TSPI.h"

#include "App.h"
#include "ui_MainWindow.h"

class ACE_Message_Block;
class QTimer;

namespace SideCar {
namespace GUI {

class MessageWriter;

namespace RangeTruthEmitter {

class SegmentModel;

class MainWindow : public MainWindowBase, private Ui::MainWindow
{
    Q_OBJECT
    using Super = MainWindowBase;
public:

    static Logger::Log& Log();

    MainWindow();

    ~MainWindow();

    App* getApp() const { return App::GetApp(); }

signals:

    void activeSegmentChanged(int value);

private slots:

    void on_port__valueChanged(int value);
    void on_systemId__valueChanged(int value);
    void on_rangeFormat__toggled(bool value);
    void on_manualRange__valueChanged(double value);
    void on_manualAzimuth__valueChanged(double value);
    void on_manualElevation__valueChanged(double value);
    void on_send__clicked();
    void on_startStop__clicked();
    void on_rewind__clicked();
    void on_rate__valueChanged(double value);
    void on_addSegment__clicked();
    void on_moveUp__clicked();
    void on_moveDown__clicked();
    void on_removeSegment__clicked();
    void on_address__editingFinished();

    void on_actionLoad__triggered();
    void on_actionSave__triggered();
    void on_actionSaveAs__triggered();
    void on_actionRevert__triggered();
    void on_actionClear__triggered();
    void on_actionStart__triggered();
    void on_actionRewind__triggered();
    void on_actionSend__triggered();

    void simulatorTimeout();
    void updateButtons();
    void openRecentFile();
    void flagDirtyWindow();

private:

    void startTimer();
    void stopTimer();
    void setTimerInterval(double hz);
    void updateTargetPositionRAE(bool send);
    void updateTargetPositionXYZ(bool send);
    void updateTargetPosition(const Messages::TSPI::Ref& msg, bool send);
    void closeEvent(QCloseEvent* event);
    void checkBeforeLoad();
    void openFile(const QString& path);
    QString strippedName(const QString& path) const;
    void updateRecentFileActions();
    void saveFile(const QString& path);
    void sendMessage(const Messages::TSPI::Ref& msg);
    void sendRangeFormat(const Messages::TSPI::Ref& msg);
    void makeWriter();
    void removeWriter();

    UDPMessageWriter* writer_;
    QTimer* timer_;
    SegmentModel* model_;
    double elapsedTime_;
    int activeSegment_;
    std::vector<double> xyz_;
    QString lastFile_;
    enum { kMaxRecentFiles = 10 };
    QAction* recentFiles_[kMaxRecentFiles];
    ACE_Message_Block* tcnMsg_;
    QString lastAddress_;
    int reportCounter_;
};

} // end namespace RangeTruthEmitter
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
