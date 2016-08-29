#include <cmath>

#include "QtCore/QFileInfo"
#include "QtCore/QSettings"

#include "ImageWriter.h"
#include "PPIWidget.h"
#include "ScreenCaptureWindow.h"

static const char* const kDuration = "ScreenCaptureDuration";
static const char* const kFrameRate = "ScreenCaptureFrameRate";
static const char* const kLastPath = "ScreenCaptureLastPath";

using namespace SideCar::GUI::PPIDisplay;

ScreenCaptureWindow::ScreenCaptureWindow(QAction* action)
    : ToolWindowBase("ScreenCapture", action), timer_(),
      writer_(new ImageWriter), display_(0)
{
    setupUi(this);
    setFixedSize();
    progressBar_->hide();

    connect(&timer_, SIGNAL(timeout()), SLOT(capture()));
    connect(writer_, SIGNAL(processedFrame()), SLOT(updateStatusBar()),
            Qt::QueuedConnection);
    connect(writer_, SIGNAL(finished()), SLOT(imageWriterDone()));

    QSettings settings;
    frameRate_->setValue(settings.value(kFrameRate, 10).toInt());
    duration_->setValue(settings.value(kDuration, 30).toInt());
    filePath_->setText(settings.value(kLastPath, "frames.miff").toString());
}

void
ScreenCaptureWindow::on_frameRate__valueChanged(int value)
{
    QSettings settings;
    settings.setValue(kFrameRate, value);
}

void
ScreenCaptureWindow::on_duration__valueChanged(int value)
{
    QSettings settings;
    settings.setValue(kDuration, value);
}

void
ScreenCaptureWindow::on_startStop__clicked()
{
    static const char* kSuffix = "miff";

    if (! display_) return;

    if (! timer_.isActive()) {
	QString filePath = QFileDialog::getSaveFileName(
	    0, "Capture File", filePath_->text(), 
	    QString("Images (*.%1)").arg(kSuffix));
	if (filePath.isEmpty()) return;

	QFileInfo fileInfo(filePath);
	if (fileInfo.suffix() != kSuffix)
	    filePath += QString(".%1").arg(kSuffix);

	QSettings settings;
	settings.setValue(kLastPath, filePath);

	filePath_->setText(filePath);
	timer_.start(int(::floor(1.0 / frameRate_->value() * 1000)));
	elapsed_.start();

	progressBar_->setValue(0);
	progressBar_->setMaximum(duration_->value());
	progressBar_->show();

	startStop_->setText("Stop");
	duration_->setEnabled(false);
	frameRate_->setEnabled(false);
    }
    else {
	stop();
    }
}

void
ScreenCaptureWindow::stop()
{
    timer_.stop();
    startStop_->setEnabled(false);
    startStop_->setText("Saving...");
    progressBar_->setValue(0);
    progressBar_->setMaximum(writer_->getQueueSize());
    writer_->start(filePath_->text());
}

void
ScreenCaptureWindow::updateStatusBar()
{
    progressBar_->setValue(progressBar_->value() + 1);
}

void
ScreenCaptureWindow::imageWriterDone()
{
    startStop_->setText("Start...");
    startStop_->setEnabled(true);
    duration_->setEnabled(true);
    frameRate_->setEnabled(true);
    progressBar_->hide();
}

void
ScreenCaptureWindow::capture()
{
    writer_->add(display_->grabFrameBuffer());
    int elapsed = elapsed_.elapsed() / 1000;
    progressBar_->setValue(elapsed);
    if (elapsed >= duration_->value())
	stop();
}
