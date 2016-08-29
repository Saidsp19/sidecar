#include "QtCore/QByteArray"
#include "QtCore/QDir"
#include "QtCore/QFile"
#include "QtCore/QSettings"
#include "QtGui/QApplication"
#include "QtGui/QCursor"
#include "QtGui/QMessageBox"
#include "QtGui/QTextDocument"

#include "GUI/LogUtils.h"

#include "App.h"
#include "NotesWindow.h"
#include "RadarSettings.h"
#include "RecordingController.h"
#include "RecordingInfo.h"
#include "ui_NotesWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

Logger::Log&
NotesWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.NotesWindow");
    return log_;
}

NotesWindow::NotesWindow(RecordingController& controller, RecordingInfo& info)
    : Super(), gui_(new Ui::NotesWindow), controller_(controller),
      info_(info)
{
    Logger::ProcLog log("NotesWindow", Log());
    LOGINFO << std::endl;
    gui_->setupUi(this);
    setVisibleAfterRestore(false);

    connect(getApp(), SIGNAL(closingAuxillaryWindows()), SLOT(close()));

    connect(gui_->recordingStop_, SIGNAL(clicked()), &controller,
            SLOT(startStop()));
    connect(gui_->actionStop_Recording, SIGNAL(triggered()), &controller,
            SLOT(startStop()));
    connect(gui_->actionUndo, SIGNAL(triggered()), gui_->notes_,
            SLOT(undo()));
    connect(gui_->actionRedo, SIGNAL(triggered()), gui_->notes_,
            SLOT(redo()));
    connect(gui_->actionCut, SIGNAL(triggered()), gui_->notes_,
            SLOT(cut()));
    connect(gui_->actionCopy, SIGNAL(triggered()), gui_->notes_,
            SLOT(copy()));
    connect(gui_->actionPaste, SIGNAL(triggered()), gui_->notes_,
            SLOT(paste()));
    connect(gui_->actionInsert_Time, SIGNAL(triggered()),
            SLOT(on_insertNow__clicked()));
    connect(gui_->actionInsert_Elapsed, SIGNAL(triggered()),
            SLOT(on_insertElapsed__clicked()));
    
    connect(gui_->notes_, SIGNAL(copyAvailable(bool)),
            gui_->actionCut, SLOT(setEnabled(bool)));
    connect(gui_->notes_, SIGNAL(copyAvailable(bool)),
            gui_->actionCopy, SLOT(setEnabled(bool)));
    connect(gui_->notes_, SIGNAL(undoAvailable(bool)),
            gui_->actionUndo, SLOT(setEnabled(bool)));
    connect(gui_->notes_, SIGNAL(redoAvailable(bool)),
            gui_->actionRedo, SLOT(setEnabled(bool)));

    gui_->actionCut->setEnabled(false);
    gui_->actionCopy->setEnabled(false);
    gui_->actionUndo->setEnabled(false);
    gui_->actionRedo->setEnabled(false);

    RadarSettings& radarSettings(App::GetApp()->getRadarSettings());

    gui_->radarTransmitting_->setChecked(radarSettings.isTransmitting());
    gui_->radarFrequency_->setValue(radarSettings.getFrequency());
    gui_->radarRotating_->setChecked(radarSettings.isRotating());
    gui_->radarRotationRate_->setValue(radarSettings.getRotationRate());
    gui_->drfmOn_->setChecked(radarSettings.isDRFMOn());
    gui_->drfmConfig_->setText(radarSettings.getDRFMConfig());
    statsChanged();

    connect(gui_->radarTransmitting_, SIGNAL(clicked()),
            SLOT(radarTransmittingChanged()));
    connect(gui_->radarFrequency_, SIGNAL(valueChanged(double)),
            &info, SLOT(infoChanged()));
    radarTransmittingChanged();

    connect(gui_->radarRotating_, SIGNAL(clicked()),
            SLOT(radarRotatingChanged()));
    connect(gui_->radarRotationRate_, SIGNAL(valueChanged(double)),
            &info, SLOT(infoChanged()));
    radarRotatingChanged();
    
    connect(gui_->drfmOn_, SIGNAL(clicked()), SLOT(drfmOnChanged()));
    connect(gui_->drfmConfig_, SIGNAL(editingFinished()), &info,
            SLOT(infoChanged()));
    drfmOnChanged();

    connect(&info, SIGNAL(statsChanged()), SLOT(statsChanged()));

    gui_->insertNow_->setEnabled(true);
    gui_->recordingStop_->setEnabled(true);
    setWindowTitle(QString("%1 Notes").arg(info.getName()));

    gui_->now_->setText(controller.getNow());
    gui_->elapsed_->setText(info_.getFormattedElapsedTime());

    QAction* insertNow = new QAction("Insert Now", this);
    connect(insertNow, SIGNAL(triggered()),
            SLOT(on_insertNow__clicked()));

    QAction* insertElapsed = new QAction("Insert Elapsed", this);
    connect(insertElapsed, SIGNAL(triggered()),
            SLOT(on_insertElapsed__clicked()));
}

void
NotesWindow::makeMenuBar()
{
    App* app = App::GetApp();
    QMenuBar* mb = menuBar();
    mb->insertMenu(mb->actions().front(), app->makeApplicationMenu(this));
    setWindowMenu(app->makeWindowMenu(this));
    mb->addMenu(getWindowMenu());
    mb->addMenu(app->makeLoggingMenu(this));
    mb->addMenu(app->makeHelpMenu(this));
}

void
NotesWindow::radarTransmittingChanged()
{
    gui_->radarFrequency_->setEnabled(gui_->radarTransmitting_->isChecked());
    info_.infoChanged();
}

void
NotesWindow::radarRotatingChanged()
{
    gui_->radarRotationRate_->setEnabled(gui_->radarRotating_->isChecked());
    info_.infoChanged();
}

void
NotesWindow::drfmOnChanged()
{
    gui_->drfmConfig_->setEnabled(gui_->drfmOn_->isChecked());
    gui_->drfmConfigLabel_->setEnabled(gui_->drfmOn_->isChecked());
    info_.infoChanged();
}

void
NotesWindow::recordingStopped()
{
    gui_->recordingInfo_->setVisible(false);
    gui_->actionStop_Recording->setEnabled(false);
    addTimeStampedEntry("*** STOPPED ***");
    saveToFile();
}

void
NotesWindow::recordingTick(const QString& now)
{
    gui_->now_->setText(now);
    gui_->elapsed_->setText(info_.getFormattedElapsedTime());
}

void
NotesWindow::on_insertNow__clicked()
{
    gui_->notes_->insertPlainText(QString("%1 ").arg(gui_->now_->text()));
    gui_->notes_->setFocus(Qt::OtherFocusReason);
}

void
NotesWindow::on_insertElapsed__clicked()
{
    gui_->notes_->insertPlainText(QString("%1 ")
                                  .arg(gui_->elapsed_->text()));
    gui_->notes_->setFocus(Qt::OtherFocusReason);
}

void
NotesWindow::addTimeStampedEntry(const QString& text)
{
    gui_->notes_->moveCursor(QTextCursor::End);
    gui_->notes_->textCursor().insertText(QString("%1 %2\n")
                                          .arg(gui_->now_->text())
                                          .arg(text));
    gui_->notes_->moveCursor(QTextCursor::End);
}

void
NotesWindow::closeEvent(QCloseEvent* event)
{
    Logger::ProcLog log("closeEvent", Log());
    LOGINFO << std::endl;
    saveToFile();
    Super::closeEvent(event);
}

void
NotesWindow::loadFromFile(QFile& file)
{
    Logger::ProcLog log("loadFromFile", Log());

    gui_->recordingInfo_->setVisible(false);
    gui_->actionStop_Recording->setEnabled(false);

    // Radar Transmitting
    //
    QByteArray data = file.readLine().trimmed();
    QList<QByteArray> bits = data.split(' ');
    if (bits.count() == 7) {
	gui_->radarTransmitting_->setChecked(true);
	gui_->radarFrequency_->setValue(bits[5].toDouble());
    }
    else {
	gui_->radarTransmitting_->setChecked(false);
    }

    // Radar Rotating
    //
    data = file.readLine().trimmed();
    bits = data.split(' ');
    if (bits.count() == 7) {
        gui_->radarRotating_->setChecked(true);
        gui_->radarRotationRate_->setValue(bits[5].toDouble());
    } else {
        gui_->radarRotating_->setChecked(false);
    }

    // Using DRFM
    //
    data = file.readLine().trimmed();
    bits = data.split(' ');
    gui_->drfmOn_->setChecked(false);
    if (bits.count() > 3) {
	gui_->drfmOn_->setChecked(true);
	QString tmp(bits[5]);
	for (int index = 6; index < bits.size(); ++index) {
	    tmp.append(' ');
	    tmp.append(bits[index]);
	}
	gui_->drfmConfig_->setText(tmp.trimmed());
    }
    else {
	gui_->drfmOn_->setChecked(false);
    }

    file.readLine().trimmed();
    gui_->notes_->document()->setPlainText(file.readAll());

    statsChanged();
}

void
NotesWindow::saveToFile()
{
    Logger::ProcLog log("saveToFile", Log());
    LOGINFO << std::endl;
    
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QByteArray data;
    data.append("Configurations: ")
	.append(info_.getConfigurationNames().join(" "))
	.append("\nDuration: ")
	.append(info_.getElapsedTime())
	.append("\nDropped Messages: ")
	.append(gui_->drops_->text())
	.append("\nDuplicate Messages: ")
	.append(gui_->duplicates_->text());
	
    data.append("\nRadar Transmitting: ")
        .append(gui_->radarTransmitting_->isChecked() ? "YES" : "NO");

    if (gui_->radarTransmitting_->isChecked())
        data.append(" - Frequency: ")
	    .append(QString::number(gui_->radarFrequency_->value()))
	    .append(" MHz");
	
    data.append("\nRadar Rotating: ")
	.append(gui_->radarRotating_->isChecked() ? "YES" : "NO");

    if (gui_->radarRotating_->isChecked())
	data.append(" - Rate: ")
	    .append(QString::number(gui_->radarRotationRate_->value()))
	    .append(" rpm");

    data.append("\nUsing DRFM: ")
	.append(gui_->drfmOn_->isChecked() ? "YES" : "NO");

    if (gui_->drfmOn_->isChecked())
	data.append(" - Configuration: ")
	    .append(gui_->drfmConfig_->text());

    data.append("\n\n");
    data.append(gui_->notes_->document()->toPlainText().toAscii());

    foreach (QString path, info_.getRecordingDirectories()) {
	QDir dir(path);
	QFile file(dir.filePath("notes.txt"));
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
	    file.write(data);
	    file.close();
	}
    }

    gui_->notes_->document()->setModified(false);

    QApplication::restoreOverrideCursor();

    LOGDEBUG << "done" << std::endl;
}

void
NotesWindow::statsChanged()
{
    gui_->drops_->setNum(info_.getDropCount());
}

bool
NotesWindow::wasRadarTransmitting() const
{
    return gui_->radarTransmitting_->isChecked();
}

double
NotesWindow::getRadarFrequency() const
{
    return gui_->radarFrequency_->value();
}

bool
NotesWindow::wasRadarRotating() const
{
    return gui_->radarRotating_->isChecked();
}

double
NotesWindow::getRadarRotationRate() const
{
    return gui_->radarRotationRate_->value();
}

bool
NotesWindow::wasDRFMOn() const
{
    return gui_->drfmOn_->isChecked();
}

QString
NotesWindow::getDRFMConfig() const
{
    return wasDRFMOn() ? gui_->drfmConfig_->text() : QString("");
}

void
NotesWindow::start(const QStringList& changes)
{
    if (! changes.isEmpty()) {
	addAlert("*** Parameter(s) Changed from Configuration:\n");
	addAlerts(changes);
    }
    addTimeStampedEntry("*** STARTED ***");
}

void
NotesWindow::addChangedParameters(const QStringList& changes)
{
    if (! changes.isEmpty()) {
	addTimeStampedEntry("*** Parameter(s) Changed:");
	addAlerts(changes);
    }
}

void
NotesWindow::addAlerts(const QStringList& alerts)
{
    foreach (QString alert, alerts)
	addAlert(alert);
}

void
NotesWindow::addAlert(const QString& alert)
{
    gui_->notes_->moveCursor(QTextCursor::End);
    gui_->notes_->textCursor().insertText(alert);
    gui_->notes_->moveCursor(QTextCursor::End);
}
