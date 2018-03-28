#ifdef USING_RTCL
#include <rtcl/Application.h>
#endif

#include <iostream>

#include "QtCore/QLibraryInfo"
#include "QtOpenGL/QGLFormat"
#include "QtOpenGL/QGLFramebufferObject"
#include "QtOpenGL/QGLPixelBuffer"

#include "App.h"
#include "GUI/LogUtils.h"

using namespace SideCar::GUI::PPIDisplay;

int
main(int argc, char* argv[])
{
#ifdef USING_RTCL

    // Initialize RTCL engine. Not sure what arguments it may take...
    //
    RTCL::Application rapp(argc, argv);
#endif

    App* app = new App(argc, argv);

    // Must have OpenGL to do anything useful
    //
    if (!QGLFormat::hasOpenGL()) {
        qWarning("This system has no OpenGL support.");
        return -1;
    }

    // Must have OpenGL pixel buffers or framebuffer objects.
    //
    if (!QGLPixelBuffer::hasOpenGLPbuffers() && !QGLFramebufferObject::hasOpenGLFramebufferObjects()) {
        qWarning("This system has no OpenGL pixel buffer or framebuffer object"
                 " support.");
        return -1;
    }

    app->restoreWindows();

    return app->exec();
}
