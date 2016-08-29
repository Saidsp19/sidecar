#ifndef SIDECAR_GUI_MASTER_LOGVIEWWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_LOGVIEWWINDOW_H

#include "QtGui/QTextCursor"
#include "QtGui/QTextDocument"

#include "GUI/MainWindowBase.h"

namespace Logger { class Log; }
namespace Ui { class LogViewWindow; }

namespace SideCar {
namespace GUI {
namespace Master {

/** Window that shows the log output for a specific remote Runner process. A RunnerLog instance creates its own
    LogViewWindow when asked to show the window by a user action; once created, the window will live until the
    application quits.
*/
class LogViewWindow : public MainWindowBase
{
    Q_OBJECT
    using Super = MainWindowBase;
public:

    /** Obtain the log device to use for LogViewWindow objects.

        \return Log device
    */
    static Logger::Log& Log();

    /** Constructor.
     */
    LogViewWindow(const QString& title, QTextDocument* logData);

    /** Notification that the managing RunnerLog instance is about to append log data to its internal
	QTextDocument. The LogViewWindow prepares for the update by determining if it should scroll to the new
	end of the document, or keep the view position as-is.
    */
    void beginUpdate();

private slots:

    /** Notification from the internal QTextEdit object that the text has changed. Manage the view position
	based on the view position found in beginUpdate().
    */
    void textChanged();

private:
    Ui::LogViewWindow* gui_;
    int vPos_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
