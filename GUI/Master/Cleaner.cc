#include <cstdlib>

#include "Cleaner.h"

using namespace SideCar::GUI::Master;

CleanerThread::CleanerThread(const QSet<QString>& hosts)
    : hosts_(hosts), stop_(false)
{
    ;
}

void
CleanerThread::stop()
{
    stop_ = true;
    wait();
}

void
CleanerThread::run()
{
    foreach(QString host, hosts_) {

	if (stop_)
	    return;

	::system(QString("ssh %1 'killall runner tail'")
                 .arg(host).toAscii());

	emit finishedHost(host);

	sleep(1);
    }
}

Cleaner::Cleaner(QWidget* parent, const QSet<QString>& hosts)
    : QObject(parent), thread_(new CleanerThread(hosts)),
      dialog_(new QProgressDialog(parent))
{
    thread_->moveToThread(thread_);

    dialog_->setAutoClose(false);
    dialog_->setAutoReset(false);
    dialog_->setRange(0, hosts.size());
    dialog_->setLabelText("Purging SideCar processes...");
    dialog_->setMinimumDuration(0);
    dialog_->setWindowModality(Qt::ApplicationModal);
    dialog_->setValue(0);

    connect(dialog_, SIGNAL(canceled()), SLOT(canceled()));
    connect(thread_, SIGNAL(finishedHost(QString)),
            SLOT(finishedHost(QString)));
    connect(thread_, SIGNAL(finished()), SLOT(finishedAll()));
}

Cleaner::~Cleaner()
{
    if (thread_) {
	thread_->stop();
	delete thread_;
    }

    if (dialog_)
	delete dialog_;
}

void
Cleaner::start()
{
    thread_->start();
}

void
Cleaner::finishedHost(QString host)
{
    if (thread_) {
	int value = dialog_->value() + 1;
	dialog_->setLabelText(
	    QString("Purged SideCar processes on %1...").arg(host));
	dialog_->setValue(value);
    }
}

void
Cleaner::finishedAll()
{
    if (thread_ && dialog_) {
	thread_->stop();
	thread_->deleteLater();
	thread_ = 0;

	dialog_->deleteLater();
	dialog_ = 0;

	emit finished();
    }
}

void
Cleaner::canceled()
{
    if (thread_ && dialog_)
	finishedAll();
}
