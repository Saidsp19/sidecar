#ifndef SIDECAR_GUI_GLINITLOCK_H // -*- C++ -*-
#define SIDECAR_GUI_GLINITLOCK_H

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class GLInitLock {
public:
    static Logger::Log& Log();

    GLInitLock();

    ~GLInitLock();

private:
    static int GetSemaphore();

    static int semaphore_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
