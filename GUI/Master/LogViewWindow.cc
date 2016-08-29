#include "QtGui/QScrollBar"
#include "QtGui/QSyntaxHighlighter"
#include "QtGui/QTextCharFormat"
#include "QtGui/QTextCursor"
#include "QtGui/QTextDocument"

#include "GUI/LogUtils.h"

#include "App.h"
#include "LogViewWindow.h"
#include "RunnerLog.h"
#include "TreeViewItem.h"
#include "ui_LogViewWindow.h"

using namespace SideCar::GUI::Master;

Logger::Log&
LogViewWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.LogViewWindow");
    return log_;
}

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
	    int index = text.indexOf(errorRegExp_);
	    while (index >= 0) {
		int length = errorRegExp_.matchedLength();
		setFormat(index, length, errorFormat_);
		index = text.indexOf(errorRegExp_, index + length);
	    }
	}
};

LogViewWindow::LogViewWindow(const QString& title, QTextDocument* logData)
    : Super(), gui_(new Ui::LogViewWindow), vPos_(-1)
{
    gui_->setupUi(this);
    setWindowTitle(title);

    connect(getApp(), SIGNAL(closingAuxillaryWindows()), SLOT(close()));

    gui_->log_->setDocument(logData);
    gui_->log_->moveCursor(QTextCursor::End);

    connect(gui_->actionCopy, SIGNAL(triggered()), gui_->log_,
            SLOT(copy()));
    connect(gui_->actionClear, SIGNAL(triggered()), gui_->log_,
            SLOT(clear()));
    connect(gui_->log_, SIGNAL(textChanged()), SLOT(textChanged()));

    // Create a new colorizer to highlight important messages.
    //
    new LogColorizer(gui_->log_);
}

void
LogViewWindow::beginUpdate()
{
    static Logger::ProcLog log("beginUpdate", Log());
    QRect cursorRect(gui_->log_->cursorRect());
    int height = gui_->log_->viewport()->height();
    LOGDEBUG << "cursorRect: " << cursorRect << " height: " << height
	     << std::endl;

    // Determine if the user has scrolled up, and if so, record the current vertical scrollbar position. We will
    // restore the scrollbar to this value from within textChanged() after the update is complete.
    //
    if (cursorRect.top() <= height)
	vPos_ = -1;
    else
	vPos_ = gui_->log_->verticalScrollBar()->value();

    LOGDEBUG << "vPos: " << vPos_ << std::endl;
}

void
LogViewWindow::textChanged()
{
    gui_->log_->moveCursor(QTextCursor::End);
    if (vPos_ != -1) {

	// Don't be obnoxious scroll to the end of the document if the user has scrolled up.
	//
	gui_->log_->verticalScrollBar()->setValue(vPos_);
    }
}
