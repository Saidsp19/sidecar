#include <cmath>

#ifdef darwin
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "QtOpenGL/QGLFramebufferObject"

#include "GUI/LogUtils.h"
#include "GUI/SampleImaging.h"

#include "OffscreenBuffer.h"

using namespace SideCar::GUI;

Logger::Log&
OffscreenBuffer::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.OffscreenBuffer");
    return log_;
}

OffscreenBuffer::OffscreenBuffer(const SampleImaging* imaging, double xMin, double xMax, double yMin, double yMax,
                                 int width, int height, int textureType) :
    imaging_(imaging),
    fbo_(0), displayLists_(0), texture_(), xMin_(xMin), xMax_(xMax), yMin_(yMin), yMax_(yMax)
{
    Logger::ProcLog log("OffscreenBuffer", Log());
    LOGINFO << "width: " << width << " height: " << height << std::endl;
    if (textureType == -1) textureType = Texture::GetBestTextureType();
    int pWidth = Texture::GetBestTextureSpan(width);
    int pHeight = Texture::GetBestTextureSpan(height);
    fbo_ = new QGLFramebufferObject(pWidth, pHeight, textureType);
    texture_ = Texture(textureType, fbo_->isValid() ? fbo_->texture() : 0, width, height);
    fbo_->bind();

    // Allocate a set of unique IDs for OpenGL display lists. Create the display lists, executing the one that
    // will clear out the framebuffer first.
    //
    displayLists_ = glGenLists(kNumLists);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    makeDisplayLists();

    fbo_->release();
}

OffscreenBuffer::~OffscreenBuffer()
{
    if (displayLists_) glDeleteLists(displayLists_, kNumLists);
    delete fbo_;
}

void
OffscreenBuffer::remakeDisplayLists()
{
    fbo_->bind();
    makeDisplayLists();
    fbo_->release();
}

void
OffscreenBuffer::makeDisplayLists()
{
    Logger::ProcLog log("makeDisplayLists", Log());
    LOGINFO << std::endl;

    fbo_->bind();

    // Create a display list that will setup the active OpenGL context for
    // drawing in our framebuffer.
    //
    GLEC(glNewList(getDisplayList(kBeginRender), GL_COMPILE));
    {
        // Hmmmm. Need this here, or else the binary video channel fails to erase! What's up with that?!
        // glDisable(GL_BLEND);
        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(0, 0, texture_.getWidth(), texture_.getHeight());
        glPushMatrix();
        glLoadIdentity();
        // gluOrtho2D(xMin_, xMax_, yMin_, yMax_);
        glOrtho(xMin_, xMax_, yMin_, yMax_, -1.0, 1.0);
    }
    GLEC(glEndList());

    // Create a display list that restores the active OpenGL context as it was before the kBeginRender display
    // list was executed. NOTE: does not restore the glPointSize setting.
    //
    GLEC(glNewList(getDisplayList(kEndRender), GL_COMPILE));
    {
        glPopMatrix();
        glPopAttrib();
    }
    GLEC(glEndList());

    fbo_->release();
}

void
OffscreenBuffer::clearBuffer()
{
    if (fbo_->isValid()) {
        fbo_->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        fbo_->release();
    }
}

void
OffscreenBuffer::renderPoints(const VertexColorArray& points)
{
    static Logger::ProcLog log("renderPoints", Log());
    if (fbo_->isValid() && !points.empty()) {
        fbo_->bind();
        GLEC(glCallList(getDisplayList(kBeginRender)));
        GLEC(glPointSize(imaging_->getSize()));
        GLEC(points.draw(GL_POINTS));
        GLEC(glCallList(getDisplayList(kEndRender)));
        fbo_->release();
    }
}

bool
OffscreenBuffer::isValid() const
{
    return fbo_->isValid();
}

void
OffscreenBuffer::setBounds(double xMin, double xMax, double yMin, double yMax)
{
    xMin_ = xMin;
    xMax_ = xMax;
    yMin_ = yMin;
    yMax_ = yMax;
    remakeDisplayLists();
}
