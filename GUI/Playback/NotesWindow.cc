#include "QtCore/QDir"
#include "QtCore/QFile"

#include "NotesWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Playback;

NotesWindow::NotesWindow(const QString& recordingDir)
    : QWidget(), Ui::NotesWindow()
{
    setupUi(this);
    setWindowTitle(QString("Notes - %1").arg(recordingDir));
    QFile file(QDir(recordingDir).filePath("notes.txt"));
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text))
	notes_->setPlainText(QString(file.readAll()));
}
