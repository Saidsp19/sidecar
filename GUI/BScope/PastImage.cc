#include <cmath>

#include "QtCore/QTimerEvent"
#include "QtGui/QApplication"
#include "QtGui/QCursor"
#include "QtGui/QMouseEvent"
#include "QtGui/QPainter"
#include "QtGui/QPen"

#include "GUI/LogUtils.h"
#include "GUI/PhantomCursorImaging.h"

#include "App.h"
#include "Configuration.h"
#include "PastImage.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::BScope;

static const int kUpdateRate = 100;

Logger::Log&
PastImage::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.PastImage");
    return log_;
}

PastImage::PastImage(QWidget* parent, const QImage& image, const QSize& size)
    : QWidget(parent), image_(image), imageSize_(size),
      phantomCursorImaging_(0),
      phantomCursor_(PhantomCursorImaging::InvalidCursor()),
      lastCursorPosition_(-1.0, -1.0), label_(""),
      showPhantomCursor_(true)
{
    Logger::ProcLog log("PastImage", Log());
    LOGINFO << "size: " << image.size() << std::endl;

    if (size.isNull())
	imageSize_ = image.size();

    setCursor(Qt::CrossCursor);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setBackgroundRole(QPalette::Dark);
    phantomCursorImaging_ =
	App::GetApp()->getConfiguration()->getPhantomCursorImaging();
    connect(phantomCursorImaging_, SIGNAL(enabledChanged(bool)),
            SLOT(update()));
    connect(phantomCursorImaging_, SIGNAL(settingChanged()),
            SLOT(update()));
    viewSettings_ =
	App::GetApp()->getConfiguration()->getViewSettings();
}

void
PastImage::setImageSize(const QSize& size)
{
    Logger::ProcLog log("setImageSize", Log());
    LOGINFO << "size: " << size << " imageSize: " << imageSize_ << std::endl;
    if (size != imageSize_) {
	imageSize_ = size;
	resize(imageSize_);
	updateGeometry();
	update();
    }
}

void
PastImage::setImage(const QImage& image)
{
    Logger::ProcLog log("setImage", Log());
    LOGINFO << "image.size: " << image.size()
	    << " imageSize: " << imageSize_ << std::endl;
    image_ = image;
    if (image.size() != imageSize_)
	setImageSize(image.size());
    update();
}

void
PastImage::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    painter.drawImage(QPoint(0, 0), image_);

    if (! label_.isEmpty()) {
	painter.setFont(QFont("Helvetica", 18, QFont::Bold, false));
	painter.setPen(Qt::white);
	painter.drawText(5, 20, label_);
    }

    if (showPhantomCursor_) {
	phantomCursorImaging_->drawCursor(painter, phantomCursor_);
    }
}

void
PastImage::setPhantomCursor(const QPointF& pos)
{
    if (! underMouse() && pos != phantomCursor_) {
	phantomCursor_ = pos;
	update();
    }
}

void
PastImage::clearPhantomCursor()
{
    phantomCursor_ = PhantomCursorImaging::InvalidCursor();
    update();
}

void
PastImage::enterEvent(QEvent* event)
{
    Super::enterEvent(event);
    clearPhantomCursor();
    checkCursorPosition();
    if (! updateTimer_.isActive())
	updateTimer_.start(kUpdateRate, this);
}

void
PastImage::leaveEvent(QEvent* event)
{
    Super::leaveEvent(event);
    clearPhantomCursor();
    if (updateTimer_.isActive())
	updateTimer_.stop();
}

void
PastImage::closeEvent(QCloseEvent* event)
{
    if (updateTimer_.isActive())
	updateTimer_.stop();
    App::GetApp()->setPhantomCursor(PhantomCursorImaging::InvalidCursor());
    Super::closeEvent(event);
}

void
PastImage::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == updateTimer_.timerId()) {
	event->accept();
	if (underMouse()) {
	    checkCursorPosition();
	}
    }
    else {
	Super::timerEvent(event);
    }
}

void
PastImage::checkCursorPosition()
{
    QPointF cursorPosition(mapFromGlobal(QCursor::pos()));
    if (cursorPosition != lastCursorPosition_) {
	lastCursorPosition_ = cursorPosition;
	cursorPosition.setX(cursorPosition.x() / (width() - 1));
	cursorPosition.setY(1.0 - cursorPosition.y() / (height() - 1));
	App::GetApp()->setPhantomCursor(cursorPosition);
    }
}

void
PastImage::setLabel(const QString& label)
{
    label_ = label;
    update();
}

void
PastImage::showPhantomCursor(bool state)
{
    if (showPhantomCursor_ != state) {
	showPhantomCursor_ = state;
	update();
    }
}
