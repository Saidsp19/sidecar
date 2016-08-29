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
#include "QtGui/QVBoxLayout"

#include "GUI/LogUtils.h"
#include "GUI/SvgIconMaker.h"
#include "GUI/ToolBar.h"

#include "App.h"
#include "Configuration.h"
#include "CursorWidget.h"
#include "FrameWidget.h"
#include "FramesWindow.h"
#include "HistorySettings.h"
#include "ImageScaler.h"
#include "MainWindow.h"
#include "PastImage.h"
#include "PlayerPositioner.h"
#include "PlayerSettings.h"
#include "PlayerWindow.h"

#include "ui_PlayerWindow.h"

using namespace SideCar::GUI::BScope;

Logger::Log&
PlayerWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.PlayerWindow");
    return log_;
}

PlayerWindow::PlayerWindow(const QList<QImage>& past, const QImage& blank,
                           int shortcut)
    : Super(), past_(past), scaled_(), frame_(), positioner_(0),
      imageScaler_(new ImageScaler(this)), playTimer_(), scale_(0.0),
      rescalingIndex_(0), panning_(false), needUpdateMaxSize_(true),
      rescalingAll_(false)
{
    Ui::PlayerWindow gui;
    gui.setupUi(this);
    actionViewTogglePhantomCursor_ = gui.actionViewTogglePhantomCursor_;

    SvgIconMaker im;
    actionViewTogglePhantomCursor_->setIcon(im.make("phantomCursor"));

    setObjectName("PlayerWindow");
    makeShowAction(shortcut);

    Configuration* configuration = App::GetApp()->getConfiguration();
    playerSettings_ = configuration->getPlayerSettings();
    connect(playerSettings_, SIGNAL(playbackRateChanged(int)),
            SLOT(playbackRateChanged(int)));
    connect(playerSettings_, SIGNAL(scaleChanged(double)),
            SLOT(setScale(double)));

    QVBoxLayout* layout = new QVBoxLayout(centralWidget());
    layout->setMargin(0);
    layout->setSpacing(0);

    scrollArea_ = new QScrollArea(centralWidget());
    layout->addWidget(scrollArea_);

    normalSize_ = blank.size();
    frame_ = new FrameWidget(scrollArea_, blank, blank.size());
    scrollArea_->setWidget(frame_);

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

    positioner_ = new PlayerPositioner(playerSettings_, toolBar);
    toolBar->addWidget(positioner_);
    connect(positioner_, SIGNAL(started()), SLOT(start()));
    connect(positioner_, SIGNAL(stopped()), SLOT(stop()));
    connect(positioner_, SIGNAL(positionChanged(int)),
            SLOT(positionerValueChanged(int)));

    connect(actionViewTogglePhantomCursor_, SIGNAL(toggled(bool)),
            frame_->getPastImage(), SLOT(showPhantomCursor(bool)));

    connect(imageScaler_, SIGNAL(done(const QImage&, int)),
            SLOT(imageScalerDone(const QImage&, int)));

    setScale(playerSettings_->getScale());
}

void
PlayerWindow::setNormalSize(const QSize& newSize)
{
    Logger::ProcLog log("setNormalSize", Log());
    LOGINFO << "newSize: " << newSize.width() << 'x' << newSize.height()
	    << std::endl;
    normalSize_ = newSize;
    setScale(scale_);
}

void
PlayerWindow::setScale(double scale)
{
    Logger::ProcLog log("setScale", Log());
    LOGINFO << "scale: " << scale << std::endl;

    scale_ = scale;
    scaledSize_ = normalSize_;
    scaledSize_ *= scale_;
    LOGDEBUG << "scaledSize: " << scaledSize_.width() << 'x'
	     << scaledSize_.height() << std::endl;

    frame_->setImageSize(scaledSize_);

    if (! past_.empty())
	rescaleImage(0, true);

    if (isVisible())
	updateMaxSize();
    else
	needUpdateMaxSize_ = true;
}

void
PlayerWindow::imageScalerDone(const QImage& image, int index)
{
    // If the given index is larger than the current rescaling index, just forget it since it is a result from a
    // stale rescaling.
    //
    if (index > rescalingIndex_)
	return;

    // Be careful: the ImageScaler thread may emit a scaled image for an entry that no longer exists.
    //
    if (index >= scaled_.size()) {
	rescalingAll_ = false;
	return;
    }

    // Save new scaled image, and update the view if user was looking at the old image.
    //
    scaled_[index] = image;
    int viewingIndex = positioner_->getPosition();
    if (viewingIndex < 0)
	viewingIndex = 0;
    if (viewingIndex == index)
	frame_->setImage(image);

    // Scale more images until done.
    //
    if (rescalingAll_) {
	++index;
	if (index < scaled_.size()) {
	    rescaleImage(index, true);
	}
	else {
	    rescalingIndex_ = 0;
	    rescalingAll_ = false;
	}
    }
}

void
PlayerWindow::setFrameCount(int frameCount)
{
    Logger::ProcLog log("setFrameCount", Log());
    LOGINFO << "frameCount: " << frameCount << std::endl;

    if (frameCount < scaled_.size()) {
	do {
	    scaled_.pop_back();
	} while (frameCount < scaled_.size());
    }

    while (frameCount > scaled_.size()) {
	int index = scaled_.size();
	scaled_.push_back(past_[index]);
	rescaleImage(index, false);
    }

    if (frameCount <= positioner_->getMaximum()) {
	positioner_->setMaximum(frameCount - 1);
	frame_->setImage(scaled_[positioner_->getPosition()]);
    }
    else {
	positioner_->setPosition(0);
    }
}

void
PlayerWindow::frameAdded(int activeFrames)
{
    Logger::ProcLog log("frameAdded", Log());
    LOGINFO << "activeFrames: " << activeFrames << std::endl;
    positioner_->setMaximum(activeFrames - 1);
    scaled_.push_front(scaled_[0]);
    scaled_.pop_back();
    rescaleImage(0, false);
    int viewingIndex = positioner_->getPosition();
    if (viewingIndex == -1)
	positioner_->setPosition(0);
    else
	frame_->setImage(scaled_[positioner_->getPosition()]);
}

void
PlayerWindow::rescaleImage(int index, bool all)
{
    rescalingIndex_ = index;
    rescalingAll_ = all;
    if (past_[index].size() == scaledSize_)
	imageScalerDone(past_[index], index);
    else
	imageScaler_->scaleImage(past_[index], scaledSize_, index);
}

void
PlayerWindow::start()
{
    if (! playTimer_.isActive()) {
	if (positioner_->getPosition() != positioner_->getMaximum())
	    positioner_->setPosition(positioner_->getMaximum());
	playTimer_.start(playerSettings_->getPlaybackRate(), this);
	positioner_->start();
    }
}

void
PlayerWindow::stop()
{
    if (playTimer_.isActive()) {
	playTimer_.stop();
	positioner_->stop();
    }
}

void
PlayerWindow::updateInfo()
{
    frame_->getPastImage()->setLabel(
	QString("T - %1").arg(positioner_->getPosition() + 1));
}

void
PlayerWindow::playbackRateChanged(int rate)
{
    bool restart = false;
    if (playTimer_.isActive()) {
	playTimer_.stop();
	restart = true;
    }

    if (restart)
	playTimer_.start(playerSettings_->getPlaybackRate(), this);
}

void
PlayerWindow::positionerValueChanged(int value)
{
    Logger::ProcLog log("positionerValueChanged", Log());
    LOGINFO << "value: " << value << std::endl;
    frame_->setImage(scaled_[value]);
    updateInfo();
}

void
PlayerWindow::on_actionViewZoomIn__triggered()
{
    playerSettings_->changeScalingPower(1);
}

void
PlayerWindow::on_actionViewZoomOut__triggered()
{
    playerSettings_->changeScalingPower(-1);
}

void
PlayerWindow::on_actionShowFramesWindow__triggered()
{
    App::GetApp()->getFramesWindow()->showAndRaise();
}

void
PlayerWindow::on_actionShowMainWindow__triggered()
{
    App::GetApp()->getMainWindow()->showAndRaise();
}

void
PlayerWindow::on_actionPresetRevert__triggered()
{
    Configuration* configuration = App::GetApp()->getConfiguration();
    configuration->restorePreset(configuration->getActivePresetIndex());
    statusBar()->showMessage("Restored current preset.", 5000);
}

void
PlayerWindow::on_actionPresetSave__triggered()
{
    Configuration* configuration = App::GetApp()->getConfiguration();
    configuration->savePreset(configuration->getActivePresetIndex());
    statusBar()->showMessage("Saved current preset.", 5000);
}

void
PlayerWindow::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == playTimer_.timerId()) {
	event->accept();
	int position = positioner_->getPosition() - 1;
	if (position == -1) {
	    if (positioner_->isLooping()) {
		positioner_->setPosition(positioner_->getMaximum());
	    }
	    else {
		stop();
	    }
	}
	else {
	    positioner_->setPosition(position);
	}
    }
    else {
	Super::timerEvent(event);
    }
}

void
PlayerWindow::showEvent(QShowEvent* event)
{
    Super::showEvent(event);
    if (needUpdateMaxSize_)
	updateMaxSize();
}

void
PlayerWindow::hideEvent(QHideEvent* event)
{
    Super::hideEvent(event);
    stop();
}

void
PlayerWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
	event->accept();
	setCursor(Qt::ClosedHandCursor);
	frame_->getPastImage()->setCursor(Qt::ClosedHandCursor);
	update();
	panFrom_ = event->pos();
	panning_ = true;
    }

    Super::mousePressEvent(event);
}

void
PlayerWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
	event->accept();
	if (panning_) {
	    panning_ = false;
	    setCursor(Qt::ArrowCursor);
	    frame_->getPastImage()->setCursor(Qt::CrossCursor);
	}
    }
    Super::mouseReleaseEvent(event);
}

void
PlayerWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (panning_) {
	setCursor(Qt::ClosedHandCursor);
	frame_->getPastImage()->setCursor(Qt::ClosedHandCursor);
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
PlayerWindow::keyReleaseEvent(QKeyEvent* event)
{
    setCursor(Qt::ArrowCursor);
    frame_->getPastImage()->setCursor(Qt::CrossCursor);
    Super::keyPressEvent(event);
}

void
PlayerWindow::updateMaxSize()
{
    Logger::ProcLog log("updateMaxSize", Log());
    LOGINFO << std::endl;

    needUpdateMaxSize_ = false;

    // Calculate the delta between the scroll area viewport and the image frame.
    //
    QSize delta(frame_->size());
    LOGDEBUG << "frame_ size: " << delta << std::endl;
    delta -= scrollArea_->maximumViewportSize();
    LOGDEBUG << "delta: " << delta << std::endl;
    QSize extra(scrollArea_->verticalScrollBar()->sizeHint().width(),
                scrollArea_->horizontalScrollBar()->sizeHint().height());
    delta += extra;
    LOGDEBUG << "delta + sbars: " << delta << std::endl;

    // Adjust our maximum window size so that we don't grow beyond the point of showing everything in the
    // viewport.
    //
    QSize min(minimumSizeHint());
    QSize max(size());
    LOGDEBUG << "current size: " << max << std::endl;
    max.setWidth(std::max(max.width() + delta.width(), min.width()));
    max.setHeight(std::max(max.height() + delta.height(), min.height()));
    LOGDEBUG << "max size: " << max << std::endl;
    setMaximumSize(max);
    LOGDEBUG << "new size: " << size() << std::endl;

    // If expanding, grow our window as well.
    //
    if (delta.width() > 0 || delta.height() > 0) {

	// To be nice to users, only grow our window if the resulting growth is still on the desktop.
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
PlayerWindow::saveToSettings(QSettings& settings)
{
    Super::saveToSettings(settings);
    settings.setValue("showPhantomCursor",
                      actionViewTogglePhantomCursor_->isChecked());
}

void
PlayerWindow::restoreFromSettings(QSettings& settings)
{
    Super::restoreFromSettings(settings);
    bool state = settings.value("showPhantomCursor", true).toBool();
    actionViewTogglePhantomCursor_->setChecked(state);
    frame_->getPastImage()->showPhantomCursor(state);
}
