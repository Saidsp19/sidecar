#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#include "Logger/Log.h"

#include "GLInitLock.h"

using namespace SideCar::GUI;

int GLInitLock::semaphore_ = -1;

Logger::Log&
GLInitLock::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.GLInitLock");
    return log_;
}

int
GLInitLock::GetSemaphore()
{
    Logger::ProcLog log("GetSemaphore", Log());

    if (semaphore_ == -1) {
        LOGWARNING << "creating semaphore 4465" << std::endl;
        int semaphore = ::semget(4465, 1, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        LOGWARNING << "semaphore: " << semaphore << std::endl;
        if (semaphore_ == -1) {
            semaphore_ = semaphore;
#ifdef linux
            union semun {
                int val;
                struct semids_ds* bf;
                unsigned short* array;
                struct seminfo* __buf;
            } sem_union;
#else
            union semun sem_union;
#endif
            sem_union.val = 1;
            LOGWARNING << "initializing to 1" << std::endl;
            ::semctl(semaphore_, 0, SETVAL, sem_union);
        }
    }

    return semaphore_;
}

GLInitLock::GLInitLock()
{
    static Logger::ProcLog log("GLInitLock", Log());
    LOGINFO << "acquiring lock" << std::endl;
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = SEM_UNDO;
    ::semop(GetSemaphore(), &op, 1);
    LOGINFO << "acquired" << std::endl;
}

GLInitLock::~GLInitLock()
{
    static Logger::ProcLog log("GLInitLock", Log());
    LOGINFO << "releasing lock" << std::endl;
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = 0;
    ::semop(GetSemaphore(), &op, 1);
    LOGINFO << "released" << std::endl;
}
