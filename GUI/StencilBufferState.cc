#include "QtOpenGL/QGLWidget"

#include "StencilBufferState.h"

using namespace SideCar::GUI;

StencilBufferState::~StencilBufferState()
{
    if (using_) glDisable(GL_STENCIL_TEST);
}

void
StencilBufferState::use()
{
    using_ = true;
    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
}
