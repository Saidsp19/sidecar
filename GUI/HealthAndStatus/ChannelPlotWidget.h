#ifndef SIDECAR_GUI_HEALTHANDSTATUS_CHANNELPLOTWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_HEALTHANDSTATUS_CHANNELPLOTWIDGET_H

#include "QtCore/QString"
#include "QtGui/QFrame"

#include "ui_ChannelPlotWidget.h"

namespace Logger { class Log; }
namespace Utils { class RunningMedian; }

namespace SideCar {
namespace GUI {

class MessageList;

namespace HealthAndStatus {

class ChannelConnection;
class ChannelPlotSettings;
class PlotDataPointDeque;

class ChannelPlotWidget : public QFrame, Ui::ChannelPlotWidget
{
    Q_OBJECT
    using Super = QFrame;
public:

    enum AlgorithmType {
	kAverage = 0,
	kMedian,
	kNumAlgorithmTypes
    };

    static Logger::Log& Log();

    static QString GetFormattedValue(double value);

    ChannelPlotWidget(ChannelConnection* channelConnection,
                      QWidget* parent = 0);

    ~ChannelPlotWidget();

    QString getFormattedDelta(double value) const;

    QString getChannelName() const { return channelName_->text(); }

    int getMessageDecimation() const { return messageDecimation_; }

    int getSampleStartIndex() const { return sampleStartIndex_; }

    int getSampleCount() const { return sampleCount_; }

    int getRunningMedianWindowSize() const;

    double getEstimatedValue() const;

public slots:

    void connected();
    
    void processIncoming(const MessageList& msgs);

    void disconnected();

    void setMessageDecimation(int value);

    void setSampleStartIndex(int value);

    void setSampleCount(int value);

    void setRunningMedianWindowSize(int value);

    void setAlgorithm(int value);

    void setSamplesColor(const QColor& value);

    void setExpectedVariance(double value);

    void setExpected(double value);

    void clearAll();

    void clearDrops();

private:

    void showConnected(bool state = true);

    void updateValueColor(QLabel* widget, bool tooBig);

    void showEvent(QShowEvent* event);

    void closeEvent(QCloseEvent* event);

    void timerEvent(QTimerEvent* event);

    void mouseDoubleClickEvent(QMouseEvent* event);

    ChannelPlotSettings* settings_;
    Utils::RunningMedian* sampleMedian_;
    int	messageDecimation_;
    int sampleStartIndex_;
    int sampleCount_;
    int algorithm_;
    int messageCounter_;
    int dropCount_;
    uint32_t lastSequenceNumber_;

    double minValue_;
    double maxValue_;
    double sampleValue_;
    double expectedValue_;

    int timerId_;
};

} // end namespace HealthAndStatus
} // end namespace GUI
} // end namespace SideCar

#endif
