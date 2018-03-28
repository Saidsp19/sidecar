#include "QtCore/QFile"
#include "QtCore/QSettings"

#include "QtGui/QCloseEvent"
#include "QtGui/QMessageBox"
#include "QtGui/QTextDocument"

#include "App.h"
#include "ConfigurationEditor.h"
#include "ConfigurationInfo.h"
#include "ui_ConfigurationEditor.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

ConfigurationEditor::ConfigurationEditor(ConfigurationInfo& info) :
    Super(), gui_(new Ui::ConfigurationEditor), info_(info)
{
    gui_->setupUi(this);
    setWindowTitle(QString("%1 Editor[*]").arg(info.getName()));

    on_actionRevert_triggered();

    connect(getApp(), SIGNAL(closingAuxillaryWindows()), SLOT(close()));
    connect(gui_->actionClose, SIGNAL(triggered()), SLOT(close()));

    connect(gui_->actionUndo, SIGNAL(triggered()), gui_->contents_, SLOT(undo()));
    connect(gui_->actionRedo, SIGNAL(triggered()), gui_->contents_, SLOT(redo()));
    connect(gui_->actionCut, SIGNAL(triggered()), gui_->contents_, SLOT(cut()));
    connect(gui_->actionCopy, SIGNAL(triggered()), gui_->contents_, SLOT(copy()));
    connect(gui_->actionPaste, SIGNAL(triggered()), gui_->contents_, SLOT(paste()));

    connect(gui_->contents_, SIGNAL(textChanged()), SLOT(textChanged()));
    connect(gui_->contents_, SIGNAL(copyAvailable(bool)), gui_->actionCut, SLOT(setEnabled(bool)));
    connect(gui_->contents_, SIGNAL(copyAvailable(bool)), gui_->actionCopy, SLOT(setEnabled(bool)));
    connect(gui_->contents_, SIGNAL(undoAvailable(bool)), gui_->actionUndo, SLOT(setEnabled(bool)));
    connect(gui_->contents_, SIGNAL(redoAvailable(bool)), gui_->actionRedo, SLOT(setEnabled(bool)));

    gui_->actionSave->setEnabled(false);
    gui_->actionRevert->setEnabled(false);
    gui_->actionCut->setEnabled(false);
    gui_->actionCopy->setEnabled(false);
    gui_->actionUndo->setEnabled(false);
    gui_->actionRedo->setEnabled(false);
}

void
ConfigurationEditor::makeMenuBar()
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
ConfigurationEditor::textChanged()
{
    bool modified = gui_->contents_->document()->isModified();
    setWindowModified(modified);
    gui_->actionSave->setEnabled(modified);
    gui_->actionRevert->setEnabled(modified);
}

void
ConfigurationEditor::on_actionRevert_triggered()
{
    QFile file(info_.getPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(qApp->activeWindow(), "Load Failed",
                              QString("<p>Unable to read from the file '%1'</p> "
                                      "<p>System error: %2</p>")
                                  .arg(file.fileName())
                                  .arg(file.errorString()));
        return;
    }

    gui_->contents_->clear();
    gui_->contents_->setPlainText(QString(file.readAll()));
    gui_->contents_->moveCursor(QTextCursor::Start);
    gui_->contents_->document()->setModified(false);
    setWindowModified(false);
    gui_->actionSave->setEnabled(false);
    gui_->actionRevert->setEnabled(false);
}

void
ConfigurationEditor::on_actionSave_triggered()
{
    save();
}

bool
ConfigurationEditor::save()
{
    QFile file(info_.getPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(qApp->activeWindow(), "Save Failed",
                              QString("<p>Unable to write to the file '%1'</p> "
                                      "<p>System error: %2</p>")
                                  .arg(file.fileName())
                                  .arg(file.errorString()));
        return true;
    }

    file.write(gui_->contents_->document()->toPlainText().toAscii());
    file.close();

    gui_->contents_->document()->setModified(false);
    setWindowModified(false);
    gui_->actionSave->setEnabled(false);
    gui_->actionRevert->setEnabled(false);

    info_.reload();
    return !info_.hasError();
}

void
ConfigurationEditor::closeEvent(QCloseEvent* event)
{
    if (gui_->contents_->document()->isModified()) {
        if (QMessageBox::information(qApp->activeWindow(), "Save Changes",
                                     QString("<p>Do you wish to save the changes to '%1'?</p>").arg(info_.getName()),
                                     QMessageBox::No | QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            if (!save()) {
                event->ignore();
                return;
            }
        }
    }

    Super::closeEvent(event);
}

void
ConfigurationEditor::setCursorPosition(int line, int column)
{
    QTextCursor cursor(gui_->contents_->document());
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
    gui_->contents_->setTextCursor(cursor);
}
