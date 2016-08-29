#include "GUI/LogUtils.h"

#include "WorkerThread.h"
#include "WorkRequest.h"

using namespace SideCar::GUI::Spectrum;

Logger::Log&
WorkerThread::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("spectrum.WorkerThread");
    return log_;
}

WorkerThread::WorkerThread(WorkRequest* workRequest)
    : Super(), workRequest_(workRequest)
{
    moveToThread(this);
    connect(this, SIGNAL(threadSubmit()), this, SLOT(threadProcess()),
            Qt::QueuedConnection);
    start();
}

void
WorkerThread::submit()
{
    emit threadSubmit();
}

void
WorkerThread::threadProcess()
{
    static Logger::ProcLog log("threadProcess", Log());
    LOGINFO << this << std::endl;
    if (workRequest_) {
	workRequest_->execute();
	emit finished();
    }
}

void
WorkerThread::run()
{
    Logger::ProcLog log("run", Log());
    LOGINFO << this << " starting" << std::endl;
    exec();
    delete workRequest_;
    workRequest_ = 0;
    LOGINFO << this << " stopped" << std::endl;
}

void
WorkerThread::stop()
{
    quit();
    wait();
}
