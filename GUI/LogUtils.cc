#ifdef darwin
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "LogUtils.h"

std::string
SideCar::GUI::GetOpenGLErrorString(int glErr)
{
    switch (glErr) {
    case GL_NO_ERROR: return "None";
    case GL_INVALID_ENUM: return "Invalid enum value";
    case GL_INVALID_VALUE: return "Value out of range";
    case GL_INVALID_OPERATION: return "Invalid operation";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "Invalid framebuffer operation";
    case GL_OUT_OF_MEMORY: return "Out of memory";
    case GL_STACK_UNDERFLOW: return "Internal stack underflow";
    case GL_STACK_OVERFLOW: return "Internal stack overflow";
    default: return "Unknown";
    }
}
