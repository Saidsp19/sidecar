#ifndef SIDECAR_GUI_MASTER_DISKSPACE_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_DISKSPACE_H

#include "QtCore/QDir"
#include "QtCore/QList"
#include "QtCore/QObject"
#include "QtCore/QThread"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace Master {

class DiskSpaceThread : public QThread
{
    Q_OBJECT
    using Super = QThread;
public:

    static Logger::Log& Log();

    DiskSpaceThread(const QString& path);

    const QString& getPath() const { return path_; }

    void getInfo(double& percentUsed, QString& freeSpace);

signals:

    void spaceUpdate(double percentUsed, QString freeSpace);

private:

    void run();

    void timerEvent(QTimerEvent* event);

    QString path_;
    int inUse_;
    QDir workingDirectory_;
    char* cpath_;
    int timerId_;

    friend class DiskSpaceMonitor;
};

class DiskSpaceMonitor : public QObject
{
    Q_OBJECT
    using Super = QWidget;
public:

    static Logger::Log& Log();

    static DiskSpaceThread* AddPath(const QString& path);

    static void Release(DiskSpaceThread* thread);

private:

    DiskSpaceMonitor();

    ~DiskSpaceMonitor();

    QList<DiskSpaceThread*> threads_;
    static DiskSpaceMonitor* singleton_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
