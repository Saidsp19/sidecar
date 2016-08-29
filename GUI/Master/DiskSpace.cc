#include <sys/param.h>
#include <sys/mount.h>

#ifdef linux
#include <sys/vfs.h>
#endif

#include <cmath>
#include <errno.h>
#include <iostream>

#include "QtCore/QDir"
#include "QtCore/QFileInfo"
#include "QtGui/QApplication"
#include "QtGui/QLabel"
#include "QtGui/QPainter"

#include "GUI/LogUtils.h"
#include "GUI/Utils.h"

#include "DiskSpace.h"

using namespace SideCar::GUI::Master;

Logger::Log&
DiskSpaceThread::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.DiskSpaceThread");
    return log_;
}

DiskSpaceThread::DiskSpaceThread(const QString& path)
    : QThread(), path_(path), inUse_(1),
      workingDirectory_(QDir::current())
{
    moveToThread(this);
    cpath_ = new char[path.size() + 1];
    strcpy(cpath_, path.toAscii());
}

void
DiskSpaceThread::run()
{
    timerId_ = startTimer(5000);
    timerEvent(0);
    exec();
    killTimer(timerId_);
    delete [] cpath_;
    cpath_ = 0;
}

void
DiskSpaceThread::getInfo(double& percentUsed, QString& freeSpace)
{
    // Move into the directory we want to test to cause any automounts to take effect.
    //
    struct statfs stats;
    QDir::setCurrent(cpath_);
    int rc = ::statfs(cpath_, &stats);
    QDir::setCurrent(workingDirectory_.absolutePath());

    if (rc == -1) {
	percentUsed = 0.0;
	freeSpace = "---";
	return;
    }

    percentUsed = 1.0 - double(stats.f_bavail) / double(stats.f_blocks);
    double free = double(stats.f_bavail) * double(stats.f_bsize);
    freeSpace = ByteAmountToString(free, 1);
}

void
DiskSpaceThread::timerEvent(QTimerEvent* event)
{
    double percentUsed;
    QString freeSpace;
    getInfo(percentUsed, freeSpace);
    emit spaceUpdate(percentUsed, freeSpace);
}

DiskSpaceMonitor* DiskSpaceMonitor::singleton_ = 0;

Logger::Log&
DiskSpaceMonitor::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.DiskSpaceMonitor");
    return log_;
}

DiskSpaceMonitor::DiskSpaceMonitor()
    : QObject(qApp), threads_()
{
    ;
}

DiskSpaceMonitor::~DiskSpaceMonitor()
{
    foreach (DiskSpaceThread* thread, threads_) {
	thread->quit();
	thread->wait();
	delete thread;
    }
}

DiskSpaceThread*
DiskSpaceMonitor::AddPath(const QString& path)
{
    if (! singleton_)
	singleton_ = new DiskSpaceMonitor;

    foreach(DiskSpaceThread* thread, singleton_->threads_) {
	if (thread->getPath() == path) {
	    ++thread->inUse_;
	    return thread;
	}
    }

    DiskSpaceThread* thread = new DiskSpaceThread(path);
    singleton_->threads_.append(thread);
    thread->start();
    return thread;
}

void
DiskSpaceMonitor::Release(DiskSpaceThread* thread)
{
    if (! singleton_ || ! thread)
	return;

    for (int index = 0; index < singleton_->threads_.size(); ++index) {
	if (thread == singleton_->threads_[index]) {
	    --thread->inUse_;
	    if (thread->inUse_ == 0) {
		singleton_->threads_.removeAt(index);
		thread->quit();
		thread->wait();
		delete thread;
	    }
	}
    }
}
