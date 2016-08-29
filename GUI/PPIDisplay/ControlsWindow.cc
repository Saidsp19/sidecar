#include "GUI/LogUtils.h"

#include "ui_ControlsWindow.h"
#include "ControlsWindow.h"
#include "History.h"
#include "HistorySettings.h"

using namespace SideCar::GUI::PPIDisplay;

static const char* const kOrdinality [] =
{
    "th",
    "st",
    "nd",
    "rd",
    "th",
};

Logger::Log&
ControlsWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ppidisplay.ControlsWindow");
    return log_;
}

ControlsWindow::ControlsWindow(int shortcut, History* history)
    : Super(shortcut), history_(history)
{
    Logger::ProcLog log("ControlsWindow", Log());
    LOGINFO << std::endl;

    // Append the history age controller and labels to the end of the window contents.
    //
    Ui::ControlsWindow gui;
    QWidget* tmp = new QWidget(this);
    gui.setupUi(tmp);
    qobject_cast<QVBoxLayout*>(layout())->addWidget(tmp);

    age_ = gui.age_;
    ageValue_ = gui.ageValue_;

    historyRetainedCountChanged(0);
    historyCurrentViewChanged(0);

    connect(history, SIGNAL(retainedCountChanged(int)),
            SLOT(historyRetainedCountChanged(int)));
    connect(history, SIGNAL(currentViewChanged(int)),
            SLOT(historyCurrentViewChanged(int)));
    connect(history, SIGNAL(currentViewAged(int)),
            SLOT(historyCurrentViewAged(int)));
    connect(age_, SIGNAL(valueChanged(int)), history,
            SLOT(showEntry(int)));
}

void
ControlsWindow::historyRetainedCountChanged(int size)
{
    Logger::ProcLog log("historyRetainedCountChanged", Log());
    LOGINFO << "size: " << size << std::endl;
    age_->setEnabled(size);
    age_->setMaximum(size);
}

void
ControlsWindow::historyCurrentViewChanged(int age)
{
    Logger::ProcLog log("historyCurrentViewChanged", Log());
    LOGINFO << "age: " << age << std::endl;

    age_->blockSignals(true);
    age_->setValue(age);
    age_->blockSignals(false);
    setAgeText(age);
}

void
ControlsWindow::historyCurrentViewAged(int age)
{
    Logger::ProcLog log("historyCurrentViewAged", Log());
    LOGINFO << "age: " << age << std::endl;
    if (age < history_->getRetentionCount()) {
	age_->blockSignals(true);
	age_->setValue(age);
	age_->blockSignals(false);
    }

    setAgeText(age);
}

void
ControlsWindow::setAgeText(int age)
{
    if (age == 0) {
	ageValue_->setText("now");
    }
    else if (age == 1) {
	ageValue_->setText("previous");
    }
    else {
	ageValue_->setText(
	    QString("%1%2 past")
	    .arg(age)
	    .arg(kOrdinality[std::min(age, 4)]));
    }
}
