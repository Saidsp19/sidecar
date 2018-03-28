#ifndef SIDECAR_GUI_PLAYBACK_NOTESWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_PLAYBACK_NOTESWINDOW_H

#include "QtGui/QWidget"

#include "ui_NotesWindow.h"

namespace SideCar {
namespace GUI {
namespace Playback {

/** Simple window which shows the contents of the 'notes.txt' file from a recording directory. Presents a
    read-only version of the file.
*/
class NotesWindow : public QWidget, private Ui::NotesWindow {
    Q_OBJECT
    using Super = QWidget;

public:
    /** Constructor. Initializes the window and loads in the 'notes.txt' file.

        \param recordingDir the directory where the notes file is located
    */
    NotesWindow(const QString& recordingDir);
};

} // end namespace Playback
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
