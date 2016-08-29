#include "QtGui/QImageReader"

#include "GUI/LogUtils.h"

#include "BackgroundImageSettings.h"

using namespace SideCar::GUI::PPIDisplay;

Logger::Log&
BackgroundImageSettings::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("ppidisplay.BackgroundImageSettings");
    return log_;
}

BackgroundImageSettings::BackgroundImageSettings(BoolSetting* enabled,
                                                 PathSetting* path,
                                                 OpacitySetting* opacity)
    : Super(enabled), path_(path), opacity_(opacity), image_(),
      filtered_()
{
    connect(path, SIGNAL(valueChanged(const QString&)), this,
            SLOT(pathChanged(const QString&)));
    connect(opacity, SIGNAL(valueChanged(double)), this,
            SLOT(update(double)));
    connect(enabled, SIGNAL(valueChanged(bool)), this,
            SLOT(detectEnabledChanged(bool)));
    if (! path_->getValue().isEmpty())
	pathChanged(path->getValue());
}

void
BackgroundImageSettings::detectEnabledChanged(bool state)
{
    emit imageChanged(filtered_);
}

void
BackgroundImageSettings::pathChanged(const QString& path)
{
    Logger::ProcLog log("pathChanged", Log());
    LOGERROR << "path: " << path << std::endl;

    QImageReader reader;
    LOGERROR << "formats: " << reader.supportedImageFormats() << std::endl;

    reader.setFormat("JPEG");
    reader.setFileName(path);
    image_ = reader.read();
    if (image_.isNull()) {
	LOGERROR << "failed read - " << reader.error() << ' '
		 << reader.errorString() << std::endl;
    }
    else {
	update(opacity_->getValue());
    }
}

void
BackgroundImageSettings::update(double opacity)
{
    Logger::ProcLog log("update", Log());
    LOGERROR << "image.isNull: " << image_.isNull() << std::endl;

    if (! filtered_.isNull())
	filtered_ = QImage();

    if (! image_.isNull()) {
	int scaled = int(::rint(opacity * 255));
	QImage alphaChannel = image_.alphaChannel();
	alphaChannel.fill(qRgb(scaled, scaled, scaled));
	filtered_ = image_;
	filtered_.setAlphaChannel(alphaChannel);
    }
    else if (! filtered_.isNull()) {
	filtered_ = QImage();
    }

    emit imageChanged(filtered_);
}
