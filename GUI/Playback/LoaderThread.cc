#include "GUI/LogUtils.h"

#include "Emitter.h"
#include "LoaderThread.h"

using namespace SideCar::GUI::Playback;

Logger::Log&
LoaderThread::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("playback.LoaderThread");
    return log_;
}

LoaderThread::LoaderThread(Emitter* emitter, const QFileInfo& fileInfo) :
    Super(), emitter_(emitter), fileInfo_(fileInfo)
{
    Logger::ProcLog log("LoaderThread", Log());
    LOGINFO << emitter->getName() << std::endl;
    moveToThread(this);
}

void
LoaderThread::run()
{
    Logger::ProcLog log("run", Log());
    LOGINFO << "begin " << emitter_->getName() << std::endl;
    emitter_->load(fileInfo_);
    LOGINFO << "end " << emitter_->getName() << std::endl;
}
