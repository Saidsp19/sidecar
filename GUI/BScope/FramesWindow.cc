#include "QtCore/QSettings"
#include "QtGui/QApplication"
#include "QtGui/QAction"
#include "QtGui/QDesktopWidget"
#include "QtGui/QHBoxLayout"
#include "QtGui/QKeyEvent"
#include "QtGui/QMouseEvent"
#include "QtGui/QScrollArea"
#include "QtGui/QScrollBar"
#include "QtGui/QStatusBar"
#include "QtGui/QToolBar"
#include "QtGui/QVBoxLayout"

#include "GUI/LogUtils.h"
#include "GUI/SvgIconMaker.h"
#include "GUI/ToolBar.h"

#include "App.h"
#include "Configuration.h"
#include "CursorWidget.h"
#include "FrameWidget.h"
#include "FramesListSettings.h"
#include "FramesPositioner.h"
#include "FramesWindow.h"
#include "HistorySettings.h"
#include "ImageScaler.h"
#include "MainWindow.h"
#include "PastImage.h"
#include "PlayerWindow.h"

#include "ui_FramesWindow.h"

using namespace SideCar::GUI::BScope;

Logger::Log&
FramesWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.FramesWindow");
    return log_;
}

FramesWindow::FramesWindow(const QList<QImage>& past, int shortcut)
    : Super(), past_(past), scaled_(), frames_(),
      imageScaler_(new ImageScaler(this)), panning_(false),
      restoreFromSettings_(true), needUpdateMaxSize_(false),
      positioned_(false), scrolled_(false), rescalingAll_(false)
{
    Ui::FramesWindow gui;
    gui.setupUi(this);
    actionViewTogglePhantomCursor_ = gui.actionViewTogglePhantomCursor_;

    SvgIconMaker im;
    actionViewTogglePhantomCursor_->setIcon(im.make("phantomCursor"));

    setObjectName("FramesWindow");

    makeShowAction(shortcut);

    QVBoxLayout* layout = new QVBoxLayout(centralWidget());
    layout->setMargin(0);
    layout->setSpacing(0);

    scrollArea_ = new QScrollArea(centralWidget());
    layout->addWidget(scrollArea_);

    framesView_ = new QWidget;
    scrollArea_->setWidget(framesView_);
    connect(scrollArea_->verticalScrollBar(), SIGNAL(valueChanged(int)),
            SLOT(verticalScrollBarChanged(int)));

    framesLayout_ = new QVBoxLayout(framesView_);
    framesLayout_->setMargin(0);
    framesLayout_->setSpacing(0);

    Configuration* configuration = App::GetApp()->getConfiguration();
    framesListSettings_ = configuration->getFramesListSettings();
    connect(framesListSettings_, SIGNAL(scaleChanged(double)),
            SLOT(setScale(double)));

    // Cursor info widget
    //
    CursorWidget* cursorWidget = new CursorWidget(statusBar());
    statusBar()->addPermanentWidget(cursorWidget);

    // Controller
    //
    ToolBar* toolBar = makeToolBar("Positioner", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    gui.actionViewZoomIn_->setIcon(QIcon(":/zoomIn.png"));
    gui.actionViewZoomOut_->setIcon(QIcon(":/zoomOut.png"));
    toolBar->addAction(gui.actionViewZoomIn_);
    toolBar->addAction(gui.actionViewZoomOut_);
    toolBar->addAction(actionViewTogglePhantomCursor_);

    positioner_ = new FramesPositioner(toolBar);
    toolBar->addWidget(positioner_);
    connect(positioner_, SIGNAL(positionChanged(int)),
            SLOT(positionerValueChanged(int)));
    connect(actionViewTogglePhantomCursor_, SIGNAL(toggled(bool)),
            SLOT(showPhantomCursor(bool)));

    setScale(framesListSettings_->getScale());
}

void
FramesWindow::setNormalSize(const QSize& newSize)
{
    normalSize_ = newSize;
    scaledSize_ = newSize;
    setScale(scale_);
}

void
FramesWindow::setScale(double scale)
{
    scale_ = scale;
    scaledSize_ = normalSize_;
    scaledSize_ *= scale_;

    for (int index = 0; index < past_.size(); ++index)
	frames_[index]->setImageSize(scaledSize_);

    if (! past_.empty())
	rescaleImage(0, true);

    if (isVisible())
	updateMaxSize();
    else
	needUpdateMaxSize_ = true;
}

void
FramesWindow::setFrameCount(int frameCount)
{
    Logger::ProcLog log("frameCountChanged", Log());
    LOGINFO << std::endl;

    // NOTE: items in framesLayout and frames_ are stored in opposite order. For the framesLayout_, the most
    // recent image is found at the beginning, while for the frames_ QList collection the most recent is at the
    // end.
    //
    while (frameCount < frames_.size()) {
	int index = frames_.size() - 1;
	delete framesLayout_->takeAt(index);
	delete frames_.takeAt(index);
	scaled_.pop_back();
    }

    while (frameCount > frames_.size()) {
	int index = frames_.size();
	scaled_.push_back(past_[index]);
	FrameWidget* frame =
	    new FrameWidget(framesView_, scaled_[index], scaledSize_);
	frames_.append(frame);
	framesLayout_->addWidget(frame);
	rescaleImage(index, false);
    }

    for (int index = 0; index < frames_.size(); ++index) {
	frames_[index]->getPastImage()->setLabel(
	    QString("T - %1").arg(index + 1));
    }

    positioner_->setMaximum(frameCount - 1);

    if (! needUpdateMaxSize_) {
	if (isVisible()) 
	    updateMaxSize();
	else
	    needUpdateMaxSize_ = true;
    }
}

void
FramesWindow::frameAdded(int activeFrames)
{
    Logger::ProcLog log("frameAdded", Log());
    LOGINFO << "activeFrames: " << activeFrames << std::endl;

    scaled_.push_front(
	past_[0].scaled(scaledSize_, Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
    scaled_.pop_back();
    for (int index = 0; index < scaled_.size(); ++index) {
	frames_[index]->setImage(scaled_[index]);
    }
}

void
FramesWindow::rescaleImage(int index, bool all)
{
    rescalingAll_ = all;

    if (index >= scaled_.size()) {
	rescalingAll_ = false;
	return;
    }

    QImage image;
    if (past_[index].size() == scaledSize_)
	image = past_[index];
    else
	image = past_[index].scaled(scaledSize_, Qt::IgnoreAspectRatio,
                                    Qt::SmoothTransformation);

    // Save new scaled image, and update the view if user was looking at the old image.
    //
    scaled_[index] = image;
    frames_[index]->setImage(image);

    // Scale more images until done.
    //
    if (rescalingAll_) {
	++index;
	if (index < scaled_.size()) {
	    rescaleImage(index, true);
	}
	else {
	    rescalingAll_ = false;
	}
    }
}

void
FramesWindow::positionerValueChanged(int value)
{
    if (! scrolled_) {
	int height = frames_[0]->size().height();
	positioned_ = true;
	scrollArea_->verticalScrollBar()->setValue(value * height);
	positioned_ = false;
    }
}

void
FramesWindow::verticalScrollBarChanged(int value)
{
    if (! positioned_) {
	QScrollBar* scrollBar = qobject_cast<QScrollBar*>(sender());
	double normalized = double(value - 1) /
	    double(scrollBar->maximum() + scrollBar->pageStep());
	int index = int(normalized * frames_.size());
	scrolled_ = true;
	positioner_->setPosition(index);
	scrolled_ = false;
    }
}

void
FramesWindow::on_actionViewZoomIn__triggered()
{
    framesListSettings_->changeScalingPower(1);
}

void
FramesWindow::on_actionViewZoomOut__triggered()
{
    framesListSettings_->changeScalingPower(-1);
}

void
FramesWindow::on_actionShowPlayerWindow__triggered()
{
    App::GetApp()->getPlayerWindow()->showAndRaise();
}

void
FramesWindow::on_actionShowMainWindow__triggered()
{
    App::GetApp()->getMainWindow()->showAndRaise();
}

void
FramesWindow::on_actionPresetRevert__triggered()
{
    Configuration* configuration = App::GetApp()->getConfiguration();
    configuration->restorePreset(configuration->getActivePresetIndex());
    statusBar()->showMessage("Restored current preset.", 5000);
}

void
FramesWindow::on_actionPresetSave__triggered()
{
    Configuration* configuration = App::GetApp()->getConfiguration();
    configuration->savePreset(configuration->getActivePresetIndex());
    statusBar()->showMessage("Saved current preset.", 5000);
}

void
FramesWindow::showEvent(QShowEvent* event)
{
    Super::showEvent(event);
    updateMaxSize();
}

void
FramesWindow::hideEvent(QHideEvent* event)
{
    Super::hideEvent(event);
}

void
FramesWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
	event->accept();
	framesView_->setCursor(Qt::ClosedHandCursor);
	useCursor(Qt::ClosedHandCursor);
	panFrom_ = event->pos();
	panning_ = true;
    }

    Super::mousePressEvent(event);
}

void
FramesWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
	event->accept();
	panning_ = false;
	framesView_->setCursor(Qt::ArrowCursor);
	useCursor(Qt::CrossCursor);
    }
    Super::mouseReleaseEvent(event);
}

void
FramesWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (panning_) {
	framesView_->setCursor(Qt::ClosedHandCursor);
	useCursor(Qt::ClosedHandCursor);
	QPoint panTo(event->pos());
	int dx = panTo.x() - panFrom_.x();
	QScrollBar* scrollBar = scrollArea_->horizontalScrollBar();
	scrollBar->setValue(scrollBar->value() - dx);
	int dy = panTo.y() - panFrom_.y();
	scrollBar = scrollArea_->verticalScrollBar();
	scrollBar->setValue(scrollBar->value() - dy);
	panFrom_ = panTo;
    }
}

void
FramesWindow::keyReleaseEvent(QKeyEvent* event)
{
    framesView_->setCursor(Qt::ArrowCursor);
    useCursor(Qt::CrossCursor);
    Super::keyPressEvent(event);
}

void
FramesWindow::useCursor(const QCursor& cursor)
{
    foreach (FrameWidget* frame, frames_)
	frame->getPastImage()->setCursor(cursor);
}

void
FramesWindow::showPhantomCursor(bool state)
{
    foreach (FrameWidget* frame, frames_)
	frame->getPastImage()->showPhantomCursor(state);
}

void
FramesWindow::updateMaxSize()
{
    Logger::ProcLog log("updateMaxSize", Log());
    LOGINFO << std::endl;

    needUpdateMaxSize_= false;

    // framesLayout_->update();
    framesView_->adjustSize();

    // Calculate the delta between the scroll area viewport and the image frame.
    //
    QSize delta(framesView_->size());
    LOGDEBUG << "frameView size: " << delta << std::endl;
    delta -= scrollArea_->maximumViewportSize();
    LOGDEBUG << "delta: " << delta << std::endl;

    // Allow extra space in case the scrollbars are visible. Use sizeHint() instead of size() because they may
    // not have an accurate size yet. How do I know this? Days of debugging...
    //
    QSize extra(scrollArea_->verticalScrollBar()->sizeHint().width(),
                scrollArea_->horizontalScrollBar()->sizeHint().height());
    LOGDEBUG << "extra: " << extra << std::endl;
    delta += extra;
    LOGDEBUG << "delta + sbars: " << delta << std::endl;

    // Adjust our maximum window size so that we don't grow beyond the point of showing everything in the
    // viewport.
    //
    QSize max(size());
    LOGDEBUG << "size: " << max << std::endl;
    max += delta;
    LOGDEBUG << "max size: " << max << std::endl;
    QSize min(minimumSizeHint());
    if (max.width() < min.width()) max.setWidth(min.width());
    if (max.height() < min.height()) max.setHeight(min.height());
    LOGDEBUG << "max size: " << max << std::endl;
    setMaximumSize(max);
    LOGDEBUG << "size: " << size() << std::endl;

    // If expanding, grow our window as well.
    //
    if (delta.width() > 0 || delta.height() > 0) {

	// Grow our window if we can do so while remaining completely on the desktop.
	//
	QDesktopWidget* desktop = QApplication::desktop();
	QRect available = desktop->availableGeometry();
	int right = frameGeometry().right();
	if (right + delta.width() > available.right())
	    delta.setWidth(std::max(0, available.right() - right));

	int bottom = frameGeometry().bottom();
	if (bottom + delta.height() > available.bottom())
	    delta.setHeight(std::max(0, available.bottom() - bottom));

	if (delta.width() > 0 || delta.height() > 0) {
	    QSize newSize(size());
	    newSize += delta;
	    LOGDEBUG << "resize: " << newSize << std::endl;
	    resize(newSize);
	    LOGDEBUG << "new size: " << size() << std::endl;
	}
    }
}

void
FramesWindow::saveToSettings(QSettings& settings)
{
    Super::saveToSettings(settings);
    settings.setValue("showPhantomCursor",
                      actionViewTogglePhantomCursor_->isChecked());
}

void
FramesWindow::restoreFromSettings(QSettings& settings)
{
    Super::restoreFromSettings(settings);
    bool state = settings.value("showPhantomCursor", true).toBool();
    actionViewTogglePhantomCursor_->setChecked(state);
    showPhantomCursor(state);
}
