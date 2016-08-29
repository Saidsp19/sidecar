#include "QtCore/QRegExp"
#include "QtGui/QScrollBar"
#include "QtGui/QTextCharFormat"
#include "QtGui/QSyntaxHighlighter"

#include "App.h"
#include "LogAlertsWindow.h"
#include "TreeViewItem.h"
#include "ui_LogAlertsWindow.h"

using namespace SideCar::GUI::Master;

struct LogColorizer : public QSyntaxHighlighter
{
    QTextCharFormat errorFormat_;
    QRegExp errorRegExp_;
    LogColorizer(QTextEdit* widget)
	: QSyntaxHighlighter(widget), errorFormat_(), errorRegExp_()
	{
	    errorFormat_.setForeground(TreeViewItem::GetFailureColor());
	    errorRegExp_ = QRegExp("\\b(FATAL|ERROR|WARNING)\\b");
	}

    void highlightBlock(const QString& text)
	{
	    int pos = text.indexOf(errorRegExp_);
	    while (pos >= 0) {
		int length = errorRegExp_.matchedLength();
		setFormat(pos, length, errorFormat_);
		pos = text.indexOf(errorRegExp_, pos + length);
	    }
	}
};

LogAlertsWindow::LogAlertsWindow()
    : Super(), gui_(new Ui::LogAlertsWindow)
{
    gui_->setupUi(this);
    setObjectName("LogAlertsWindow");
    makeShowAction(Qt::Key_F3);
    gui_->alerts_->document()->setMaximumBlockCount(1000);

    QFont font;
    font.setPointSize(10);
    gui_->alerts_->document()->setDefaultFont(font);
    gui_->alerts_->document()->setUndoRedoEnabled(false);

    connect(gui_->actionCopy, SIGNAL(triggered()), gui_->alerts_,
            SLOT(copy()));
    connect(gui_->actionClear, SIGNAL(triggered()), gui_->alerts_,
            SLOT(clear()));

    new LogColorizer(gui_->alerts_);
}

void
LogAlertsWindow::addAlerts(const QStringList& entries)
{
    // If the user has scrolled up to look at an earlier entry, then record the current vertical scrollbar
    // position for restoration later after appending the alert entries.
    //
    int vPos = -1;
    if (gui_->alerts_->cursorRect().top() >
        gui_->alerts_->viewport()->height() && isVisible()) {
	vPos = gui_->alerts_->verticalScrollBar()->value();
    }

    // Add the entries to the end of the buffer.
    //
    foreach (QString entry, entries) {
	gui_->alerts_->textCursor().insertText(entry);
	gui_->alerts_->moveCursor(QTextCursor::End);
    }

    // Show and raise the window if not visible, but don't be obnoxious and do that if the window is visible,
    // but not active.
    //
    if (! isVisible()) {
	show();
	raise();
    }
    else if (vPos != -1) {

	// User has scrolled up. Don't be obnoxious and scroll to the end.
	//
	gui_->alerts_->verticalScrollBar()->setValue(vPos);
    }
}
