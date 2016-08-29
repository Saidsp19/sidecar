#include "GUI/LogUtils.h"

#include "ChannelPlotSettings.h"
#include "ChannelPlotWidget.h"
#include "ConfigurationWindow.h"

#include "ui_ChannelPlotConfiguration.h"

using namespace SideCar::GUI::HealthAndStatus;

Logger::Log&
ChannelPlotSettings::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("hands.ChannelPlotSettings");
    return log_;
}

ChannelPlotSettings::ChannelPlotSettings(PresetManager* presetManager,
                                         QObject* parent,
                                         const QString& name)
    : Super(parent), name_(name),
      messageDecimation_(presetManager, name + "Decimation", 15),
      sampleStartIndex_(presetManager, name + "SampleStart", -1),
      sampleCount_(presetManager, name + "SampleCount", 100),
      runningMedianWindowSize_(presetManager, name + "RunningMedianWindowSize",
                               15),
      algorithm_(presetManager, name + "Algorithm", 0),
      samplesColor_(presetManager, name + "SamplesColor", Qt::white),
      expectedVariance_(presetManager, name + "ExpectedVariance", 100.0),
      expected_(presetManager, name + "Expected", 0.0), widget_(0)
{
    Logger::ProcLog log("ChannelPlotSettings", Log());
    LOGINFO << "name: " << name << std::endl;
    add(&messageDecimation_);
    LOGDEBUG << "messageDecimation_: " << messageDecimation_.getValue()
	     << std::endl;
    add(&sampleStartIndex_);
    LOGDEBUG << "sampleStartIndex: " << sampleStartIndex_.getValue()
	     << std::endl;
    add(&sampleCount_);
    LOGDEBUG << "sampleCount: " << sampleCount_.getValue() << std::endl;
    add(&runningMedianWindowSize_);
    LOGDEBUG << "runningMedianWindowSize: "
	     << runningMedianWindowSize_.getValue() << std::endl;
    add(&algorithm_);
    LOGDEBUG << "algorithm: " << algorithm_.getValue() << std::endl;
    add(&samplesColor_);
    LOGDEBUG << "samplesColor: " << samplesColor_.getValue().name()
	     << std::endl;
    add(&expectedVariance_);
    LOGDEBUG << "expectedVariance: " << expectedVariance_.getValue()
	     << std::endl;
    add(&expected_);
    LOGDEBUG << "expected: " << expected_.getValue()
	     << std::endl;
}

void
ChannelPlotSettings::connectToWidget(ChannelPlotWidget* widget)
{
    widget_ = widget;
    connect(&messageDecimation_, SIGNAL(valueChanged(int)), widget,
            SLOT(setMessageDecimation(int)));
    connect(&sampleStartIndex_, SIGNAL(valueChanged(int)), widget,
            SLOT(setSampleStartIndex(int)));
    connect(&sampleCount_, SIGNAL(valueChanged(int)), widget,
            SLOT(setSampleCount(int)));
    connect(&runningMedianWindowSize_, SIGNAL(valueChanged(int)),
            widget, SLOT(setRunningMedianWindowSize(int)));
    connect(&algorithm_, SIGNAL(valueChanged(int)), widget,
            SLOT(setAlgorithm(int)));
    connect(&samplesColor_, SIGNAL(valueChanged(const QColor&)),
            widget, SLOT(setSamplesColor(const QColor&)));
    connect(&expectedVariance_, SIGNAL(valueChanged(double)), widget,
            SLOT(setExpectedVariance(double)));
    connect(&expected_, SIGNAL(valueChanged(double)), widget,
            SLOT(setExpected(double)));
}

void
ChannelPlotSettings::connectToEditor(ConfigurationWindow* editor)
{
    connect(editor->samplesColor_, SIGNAL(colorChanged(const QColor&)),
            &samplesColor_, SLOT(setValue(const QColor&)));
    connect(editor->messageDecimation_, SIGNAL(valueChanged(int)),
            &messageDecimation_, SLOT(setValue(int)));
    connect(editor->sampleStartIndex_, SIGNAL(valueChanged(int)),
            &sampleStartIndex_, SLOT(setValue(int)));
    connect(editor->sampleCount_, SIGNAL(valueChanged(int)),
            &sampleCount_, SLOT(setValue(int)));
    connect(editor->runningMedianWindowSize_, SIGNAL(valueChanged(int)),
            &runningMedianWindowSize_, SLOT(setValue(int)));
    connect(editor->algorithm_, SIGNAL(currentIndexChanged(int)),
            &algorithm_, SLOT(setValue(int)));
    connect(editor->expectedVariance_, SIGNAL(valueChanged(double)),
            &expectedVariance_, SLOT(setValue(double)));
    connect(editor->expected_, SIGNAL(valueChanged(double)),
            &expected_, SLOT(setValue(double)));
}

void
ChannelPlotSettings::disconnectFromEditor(ConfigurationWindow* editor)
{
    disconnect(editor->samplesColor_, SIGNAL(colorChanged(const QColor&)),
               &samplesColor_, SLOT(setValue(const QColor&)));
    disconnect(editor->messageDecimation_, SIGNAL(valueChanged(int)),
               &messageDecimation_, SLOT(setValue(int)));
    disconnect(editor->sampleStartIndex_, SIGNAL(valueChanged(int)),
               &sampleStartIndex_, SLOT(setValue(int)));
    disconnect(editor->sampleCount_, SIGNAL(valueChanged(int)),
               &sampleCount_, SLOT(setValue(int)));
    disconnect(editor->runningMedianWindowSize_, SIGNAL(valueChanged(int)),
               &runningMedianWindowSize_, SLOT(setValue(int)));
    disconnect(editor->algorithm_, SIGNAL(currentIndexChanged(int)),
               &algorithm_, SLOT(setValue(int)));
    disconnect(editor->expectedVariance_, SIGNAL(valueChanged(double)),
               &expectedVariance_, SLOT(setValue(double)));
    disconnect(editor->expected_, SIGNAL(valueChanged(double)),
               &expected_, SLOT(setValue(double)));
}    

void
ChannelPlotSettings::copyFrom(ChannelPlotSettings* settings)
{
    messageDecimation_.setValue(settings->getMessageDecimation());
    sampleStartIndex_.setValue(settings->getSampleStartIndex());
    sampleCount_.setValue(settings->getSampleCount());
    runningMedianWindowSize_.setValue(settings->getRunningMedianWindowSize());
    algorithm_.setValue(settings->getAlgorithm());
    samplesColor_.setValue(settings->getSamplesColor());
    expectedVariance_.setValue(settings->getExpectedVariance());
    expected_.setValue(settings->getExpected());
}

double
ChannelPlotSettings::getCurrentValue() const
{
    return widget_ ? widget_->getEstimatedValue() : 0.0;
}
