#include "GUI/LogUtils.h"

#include "History.h"
#include "HistoryControlWindow.h"
#include "HistoryPosition.h"
#include "MainWindow.h"

using namespace SideCar;
using namespace SideCar::GUI::AScope;

Logger::Log&
HistoryControlWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.HistoryControlWindow");
    return log_;
}

HistoryControlWindow::HistoryControlWindow(int shortcut, const History* history) :
    ToolWindowBase("HistoryControlWindow", "History Controls", shortcut), Ui::HistoryControlWindow(), history_(history),
    historyPosition_(0)
{
    setupUi(this);
    installHistoryPosition(0);
    setFixedHeight(sizeHint().height());
    slider_->setMinimum(0);
    slider_->setMaximum(0);
}

void
HistoryControlWindow::activeMainWindowChanged(MainWindowBase* base)
{
    MainWindow* window = static_cast<MainWindow*>(base);
    installHistoryPosition(window ? window->getHistoryPosition() : 0);
}

void
HistoryControlWindow::installHistoryPosition(HistoryPosition* historyPosition)
{
    if (historyPosition_) historyPosition_->disconnect(this);

    historyPosition_ = historyPosition;

    if (historyPosition_) {
        int position = historyPosition->getPosition();
        setPosition(position);
        connect(historyPosition_, SIGNAL(positionChanged(int, double)),
                SLOT(historyPositionPositionChanged(int, double)));
        connect(historyPosition_, SIGNAL(viewChanged(const HistoryFrame*)),
                SLOT(historyPositionViewChanged(const HistoryFrame*)));

        historyPositionViewChanged(history_->getFrame(position));
        historyPositionPositionChanged(position, historyPosition_->getAge());
    } else {
        current_->setEnabled(false);
        slider_->setEnabled(false);
    }
}

void
HistoryControlWindow::setPosition(int position)
{
    current_->setEnabled(position);
    int frameCount = history_->getFrameCount();
    slider_->setEnabled(history_->isEnabled() && frameCount);
    if (position > frameCount) position = frameCount;
    slider_->setValue(position);
}

void
HistoryControlWindow::on_current__clicked()
{
    static Logger::ProcLog log("on_current__clicked", Log());
    LOGINFO << std::endl;
    slider_->setValue(0);
    historyPosition_->setPosition(0);
}

void
HistoryControlWindow::on_slider__actionTriggered(int action)
{
    static Logger::ProcLog log("on_slider__actionTriggered", Log());
    LOGINFO << "action: " << action << std::endl;

    int frameCount = history_->getFrameCount();
    int position = historyPosition_->getPosition();

    switch (action) {
    case QAbstractSlider::SliderSingleStepAdd:
        if (position < frameCount) historyPosition_->showOlderBySequence();
        break;

    case QAbstractSlider::SliderSingleStepSub:
        if (position > 0) historyPosition_->showNewerBySequence();
        break;

    case QAbstractSlider::SliderPageStepAdd: historyPosition_->showOlderByRotation(); break;

    case QAbstractSlider::SliderPageStepSub: historyPosition_->showNewerByRotation(); break;

    case QAbstractSlider::SliderMove: historyPosition_->setPosition(slider_->sliderPosition()); break;
    }
}

void
HistoryControlWindow::historyPositionPositionChanged(int position, double age)
{
    static Logger::ProcLog log("historyPositionValueChanged", Log());
    LOGINFO << "position: " << position << " age: " << age << std::endl;

    int frameCount = history_->getFrameCount();
    slider_->setMaximum(frameCount);
    setPosition(position);

    age_->setText(QString::number(age, 'f', 1));
    frameIndex_->setText(QString::number(position));
    frameCount_->setText(QString::number(frameCount));
}

void
HistoryControlWindow::historyPositionViewChanged(const HistoryFrame* frame)
{
    int frameCount = history_->getFrameCount();
    frameCount_->setText(QString::number(frameCount));
    slider_->setMaximum(frameCount);
    slider_->setEnabled(history_->isEnabled() && frameCount);
    if (frame->size() == 0) return;
    Messages::PRIMessage::Ref msg = frame->getLastMessage();
    if (!msg) {
        sequence_->setText("");
        shaft_->setText("");
    } else {
        sequence_->setText(QString::number(msg->getSequenceCounter()));
        shaft_->setText(QString::number(msg->getShaftEncoding()));
    }
}
