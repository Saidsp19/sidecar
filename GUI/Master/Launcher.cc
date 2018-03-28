#include <cstdlib>

#include "GUI/LogUtils.h"

#include "Configuration/Loader.h"
#include "Configuration/RunnerConfig.h"
#include "Launcher.h"

using namespace SideCar::Configuration;
using namespace SideCar::GUI::Master;

Logger::Log&
Launcher::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.Launcher");
    return log_;
}

Launcher::Launcher(QWidget* parent, const Loader& loader) :
    QObject(parent), loader_(loader), dialog_(new QProgressDialog(parent)), index_(0), timerId_(0)
{
    Logger::ProcLog log("Launcher", Log());
    LOGINFO << std::endl;
    dialog_->setRange(0, loader.getNumRunnerConfigs());
    dialog_->setMinimumDuration(0);
    dialog_->setWindowModality(Qt::ApplicationModal);
    dialog_->setValue(0);
    dialog_->setAutoClose(false);
    dialog_->setAutoReset(false);
    connect(dialog_, SIGNAL(canceled()), SLOT(canceled()));
    LOGINFO << "done" << std::endl;
}

void
Launcher::start()
{
    Logger::ProcLog log("start", Log());
    LOGINFO << std::endl;
    timerId_ = startTimer(500);
    timerEvent(0);
    LOGINFO << "done" << std::endl;
}

void
Launcher::canceled()
{
    Logger::ProcLog log("canceled", Log());
    LOGINFO << std::endl;

    if (dialog_) {
        QProgressDialog* tmp = dialog_;
        dialog_ = 0;
        tmp->disconnect(this);
        tmp->close();
        tmp->deleteLater();
        killTimer(timerId_);
        timerId_ = 0;
    }

    emit finished(true);
    LOGINFO << "done" << std::endl;
}

void
Launcher::timerEvent(QTimerEvent* event)
{
    Logger::ProcLog log("timerEvent", Log());
    LOGINFO << std::endl;

    if (!timerId_) {
        LOGERROR << "no timer" << std::endl;
        return;
    }

    if (!dialog_) {
        LOGERROR << "no dialog window" << std::endl;
        return;
    }

    LOGINFO << "index: " << index_ << " numRunners: " << loader_.getNumRunnerConfigs() << std::endl;

    if (index_ < loader_.getNumRunnerConfigs()) {
        const RunnerConfig* config = loader_.getRunnerConfig(index_++);
        dialog_->setLabelText(QString("Launching runner '%1' on %2...\n[%3 of %4]")
                                  .arg(config->getRunnerName())
                                  .arg(config->getHostName())
                                  .arg(index_)
                                  .arg(loader_.getNumRunnerConfigs()));

        // !!! Protect against reentrancy by temporarily setting dialog_ to NULL before we call its setValue()
        // !!! method. Close reading of the QProgressDialog documentation tells us that if QProgressDialog is
        // !!! modal (it is here), calling its setValue() method will invoke QApp::processEvents() method, which
        // !!! may call this routine while we are in it.
        //
        {
            QProgressDialog* tmp = 0;
            std::swap(tmp, dialog_);
            tmp->setValue(index_);
            std::swap(tmp, dialog_);
        }

        QString cmd(config->getRemoteCommand());
        LOGWARNING << "executing '" << cmd << "'" << std::endl;
        ::system(cmd.toStdString().c_str());

        return;
    }

    QProgressDialog* tmp = dialog_;
    dialog_ = 0;
    if (tmp) {
        tmp->disconnect(this); // !!! Do this or else canceled() gets called
        delete tmp;
    }

    killTimer(timerId_);
    timerId_ = 0;

    emit finished(false);

    LOGINFO << "done" << std::endl;
}
