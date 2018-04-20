#include "QtCore/QtGlobal" // need Q_WS_X11 definition

// On X11 we need to do some preprocessor setup to get the C prototype for the glFramebufferTexture2DEXT()
// function and the framebuffer extension constants.
//

#ifdef Q_WS_X11

// Get glFramebufferTexture2DEXT() prototype and framebuffer constants
//

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_PROTOTYPES

// If the OpenGL files do not define the framebuffer extension, do a manual define using information from Qt's
// OpenGL interface.
//

#ifndef GL_EXT_framebuffer_object

// We need this below when we attempt to resolve the glFramebufferTexture2DEXT symbol in the GL library.
//
#include "QtCore/QLibrary"

// The following was copied from Qt's qglframebufferobject.cpp file
//

#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT 0x0506
#define GL_MAX_RENDERBUFFER_SIZE_EXT 0x84E8
#define GL_FRAMEBUFFER_BINDING_EXT 0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT 0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT 0x8CD8
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT 0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT 0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT 0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS_EXT 0x8CDF
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT 0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT 0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT 0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT 0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT 0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT 0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT 0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT 0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT 0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT 0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT 0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT 0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT 0x8CED
#define GL_COLOR_ATTACHMENT14_EXT 0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT 0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT 0x8D20
#define GL_FRAMEBUFFER_EXT 0x8D40
#define GL_RENDERBUFFER_EXT 0x8D41
#define GL_RENDERBUFFER_WIDTH_EXT 0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT 0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT 0x8D44
#define GL_STENCIL_INDEX_EXT 0x8D45
#define GL_STENCIL_INDEX1_EXT 0x8D46
#define GL_STENCIL_INDEX4_EXT 0x8D47
#define GL_STENCIL_INDEX8_EXT 0x8D48
#define GL_STENCIL_INDEX16_EXT 0x8D49
#define GL_RENDERBUFFER_RED_SIZE_EXT 0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT 0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT 0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT 0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT 0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT 0x8D55

// Define a function pointer type with the same type declaration as the glFramebufferTexture2DEXT() function.
//
using PFNGLFRAMEBUFFERTEXTURE2DEXTPROC = void (*)(GLenum, GLenum, GLenum, GLuint, GLint);

// Allocate space for the result of resolving the glFramebufferTexture2DEXT symbol. If it exists, it is a
// pointer to the glFramebufferTexture2DEXT function.
//
static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT_ = 0;

#endif // ! GL_EXT_framebuffer_object
#endif // Q_WS_X11

#include "QtOpenGL/QGLFramebufferObject"

#include "GUI/GLInitLock.h"
#include "GUI/LogUtils.h"
#include "GUI/VertexColorArray.h"
#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "FFTSettings.h"
#include "SpectrographImaging.h"
#include "SpectrographWidget.h"

using namespace Utils;
using namespace SideCar::GUI::Spectrum;

static int kUpdateRate = 33; // msecs between update() calls (~30 FPS)

Logger::Log&
SpectrographWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("spectrum.SpectrographWidget");
    return log_;
}

QGLFormat
SpectrographWidget::GetGLFormat()
{
    QGLFormat format = QGLFormat::defaultFormat();
    format.setAlpha(false);
    format.setAccum(false);
    format.setDepth(false);
    format.setDoubleBuffer(true);
    format.setSampleBuffers(false);
    format.setStencil(false);
    return format;
}

SpectrographWidget::SpectrographWidget(QWidget* parent) :
    Super(GetGLFormat(), parent), imaging_(0), fbo_(0), readTexture_(0), writeTexture_(1), points_(), updateTimer_(),
    mouse_(), displayLists_(0), needUpdate_(true), doClear_(false), frozen_(false)
{
    Logger::ProcLog log("SpectrographWidget", Log());
    LOGINFO << std::endl;

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    setCursor(Qt::CrossCursor);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setBackgroundRole(QPalette::Dark);

#ifndef GL_EXT_framebuffer_object

    // Resolve the glFramebufferTexture2DEXT symbol
    //
    if (!glFramebufferTexture2DEXT_) {
        QLibrary lib(QLatin1String("GL"));
        glFramebufferTexture2DEXT_ = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)lib.resolve("glFramebufferTexture2DEXT");
    }

#define glFramebufferTexture2DEXT glFramebufferTexture2DEXT_

#endif // ! GL_EXT_framebuffer_object

    Configuration* cfg = App::GetApp()->getConfiguration();
    imaging_ = cfg->getSpectrographImaging();
    connect(imaging_, SIGNAL(historySizeChanged(int)), SLOT(sizeChanged()));
    fftSettings_ = cfg->getFFTSettings();
    connect(fftSettings_, SIGNAL(fftSizeChanged(int)), SLOT(sizeChanged()));

    sizeChanged();
}

SpectrographWidget::~SpectrographWidget()
{
    deleteOffscreenBuffer();
    glDeleteLists(displayLists_, kNumLists);
}

void
SpectrographWidget::sizeChanged()
{
    size_ = QSize(fftSettings_->getFFTSize(), imaging_->getHistorySize());
    setMinimumHeight(size_.height());
    setMaximumHeight(size_.height());
    if (fbo_) {
        deleteOffscreenBuffer();
        makeOffscreenBuffer();
    }
}

void
SpectrographWidget::initializeGL()
{
    Logger::ProcLog log("initializeGL", Log());
    LOGERROR << "*** " << fbo_ << std::endl;

    if (fbo_) return;

    displayLists_ = glGenLists(kNumLists);

    // Turn on buffer multisampling for the best imaging. Only works if QGLFormat has sample buffers enabled
    // above. glEnable(GL_MULTISAMPLE);

    // Enable the use of vertex and color arrays when calling glDrawArrays(). Additional setup happens inside
    // tthe VertexColorArray::draw() method.
    //
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // Create the interleaved Vertex and Color object array. This will call glVertexPointer and glColorPointer
    // with the appropriate pointer values.
    //
    points_.checkCapacity(10000);

    makeOffscreenBuffer();
}

void
SpectrographWidget::resizeGL(int width, int height)
{
    Super::resizeGL(width, height);
    makeDisplayLists();
}

void
SpectrographWidget::deleteOffscreenBuffer()
{
    if (fbo_) {
        GLuint id = textures_[1].getId();
        glDeleteTextures(1, &id);
        delete fbo_;
        fbo_ = 0;
    }
}

void
SpectrographWidget::makeOffscreenBuffer()
{
    GLenum textureType = Texture::GetBestTextureType();
    glEnable(textureType);

    fbo_ = new QGLFramebufferObject(size_, textureType);
    if (!fbo_->isValid()) return;
    fbo_->bind();

    textures_[0] = Texture(textureType, fbo_->texture(), size_.width(), size_.height());

    // Create a second texture which will get swapped with the one above with each render pass. Initialize to be
    // the same as the one above.
    //
    GLuint twin;
    glGenTextures(1, &twin);

    glBindTexture(textureType, twin);
    glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(textureType, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(textureType, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(textureType, 0, GL_RGBA, size_.width(), size_.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    textures_[1] = Texture(textureType, twin, size_.width(), size_.height());

    // Unbind the texture from the context.
    //
    glBindTexture(textureType, 0);

    // Clear the textures.
    //
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, textureType, textures_[1].getId(), 0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, textureType, textures_[0].getId(), 0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, textureType, textures_[1].getId(), 0);

    makeDisplayLists();
}

void
SpectrographWidget::makeDisplayLists()
{
    Logger::ProcLog log("makeDisplayLists", Log());

    makeCurrent();

    GLEC(glNewList(getDisplayList(kBeginUpdate), GL_COMPILE));
    {
        GLEC(glViewport(0, 0, size_.width(), size_.height()));
        GLEC(glMatrixMode(GL_PROJECTION));
        GLEC(glLoadIdentity());
        GLEC(glOrtho(0, size_.width(), 0, size_.height(), -1.0, 1.0));
        GLEC(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
        GLEC(glClear(GL_COLOR_BUFFER_BIT));
    }
    GLEC(glEndList());

    GLEC(glNewList(getDisplayList(kBeginPaint), GL_COMPILE));
    {
        GLEC(glViewport(0, 0, width(), size_.height()));
        GLEC(glMatrixMode(GL_PROJECTION));
        GLEC(glLoadIdentity());
        GLEC(glOrtho(0, width(), 0, size_.height(), -1.0, 1.0));
        GLEC(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GLEC(glClear(GL_COLOR_BUFFER_BIT));
    }
    GLEC(glEndList());

    for (int index = 0; index < 2; ++index) {
        GLEC(glNewList(getDisplayList(kCopyPrevious0 + index), GL_COMPILE));
        {
            const Texture& t(textures_[index]);
            GLEC(glBindTexture(t.getType(), t.getId()));
            GLEC(glBegin(GL_QUADS));
            GLEC(glTexCoord2f(t.getXMin(), t.getYMin()));
            GLEC(glVertex2f(0.0, 1.0));
            GLEC(glTexCoord2f(t.getXMax(), t.getYMin()));
            GLEC(glVertex2f(size_.width(), 1.0));
            GLEC(glTexCoord2f(t.getXMax(), t.getYMax()));
            GLEC(glVertex2f(size_.width(), size_.height() + 1.0));
            GLEC(glTexCoord2f(t.getXMin(), t.getYMax()));
            GLEC(glVertex2f(0.0, size_.height() + 1.0));
            GLEC(glEnd());
            GLEC(glBindTexture(t.getType(), 0));
        }
        GLEC(glEndList());

        GLEC(glNewList(getDisplayList(kPaintTexture0 + index), GL_COMPILE));
        {
            const Texture& t(textures_[0]);
            GLEC(glBindTexture(t.getType(), t.getId()));
            GLEC(glBegin(GL_QUADS));
            GLEC(glTexCoord2f(t.getXMin(), t.getYMin()));
            GLEC(glVertex2f(0.0, 0.0));
            GLEC(glTexCoord2f(t.getXMax(), t.getYMin()));
            GLEC(glVertex2f(width(), 0.0));
            GLEC(glTexCoord2f(t.getXMax(), t.getYMax()));
            GLEC(glVertex2f(width(), size_.height()));
            GLEC(glTexCoord2f(t.getXMin(), t.getYMax()));
            GLEC(glVertex2f(0.0, size_.height()));
            GLEC(glEnd());
            GLEC(glBindTexture(t.getType(), 0));
        }
        GLEC(glEndList());
    }
}

void
SpectrographWidget::paintGL()
{
    Logger::ProcLog log("paintGL", Log());

    needUpdate_ = false;
    GLEC(glCallList(getDisplayList(kBeginPaint)));
    if (fbo_->isValid()) {
        if (!doClear_) { GLEC(glCallList(getDisplayList(kPaintTexture0 + readTexture_))); }
    }
}

void
SpectrographWidget::clear()
{
    doClear_ = true;
    needUpdate();
}

void
SpectrographWidget::timerEvent(QTimerEvent* event)
{
    static Logger::ProcLog log("timerEvent", Log());

    if (event->timerId() != updateTimer_.timerId()) {
        event->ignore();
        Super::timerEvent(event);
        return;
    }

    if (needUpdate_) update();

    if (underMouse()) {
        QPoint newMouse = mapFromGlobal(QCursor::pos());
        if (newMouse != mouse_) {
            mouse_ = newMouse;
            emit currentCursorPosition(mouse_);
        }
    }
}

void
SpectrographWidget::needUpdate()
{
    needUpdate_ = true;
}

void
SpectrographWidget::showEvent(QShowEvent* event)
{
    Logger::ProcLog log("showEvent", Log());
    LOGINFO << std::endl;
    if (!updateTimer_.isActive()) updateTimer_.start(kUpdateRate, this);
    Super::showEvent(event);
}

void
SpectrographWidget::closeEvent(QCloseEvent* event)
{
    Logger::ProcLog log("closeEvent", Log());
    LOGINFO << std::endl;
    if (updateTimer_.isActive()) updateTimer_.stop();
    Super::closeEvent(event);
}

void
SpectrographWidget::processBins(const QVector<QPointF>& bins)
{
    if (frozen_) return;

    needUpdate();
    makeCurrent();

    if (!fbo_ || !fbo_->isValid()) return;
    fbo_->bind();

    // Attach the current write texture for imaging.
    //
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, textures_[writeTexture_].getType(),
                              textures_[writeTexture_].getId(), 0);

    // Setup view transforms.
    //
    glCallList(getDisplayList(kBeginUpdate));
    if (doClear_) {
        doClear_ = false;
    } else {
        glCallList(getDisplayList(kCopyPrevious0 + readTexture_));
    }

    // Now draw the new data at the bottom of the view.
    //
    GLfloat scale = GLfloat(size_.width()) / GLfloat(bins.size());
    GLfloat pointSize = scale * imaging_->getSize();
    glPointSize(pointSize);
    GLfloat xOffset = 0.0;
    if (pointSize > 1.0) xOffset = pointSize / 2.0;

    points_.clear();
    for (int index = 0; index < bins.size(); ++index) {
        points_.push_back(Vertex(index * scale + xOffset, 1.0), imaging_->getColor(bins[index].y()));
    }

    points_.draw(GL_POINTS);

    // Swap the buffers, and switch to the new write texture to flush the changes made above to the new read
    // texture.
    //
    readTexture_ = writeTexture_;
    writeTexture_ = 1 - writeTexture_;
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, textures_[writeTexture_].getType(),
                              textures_[writeTexture_].getId(), 0);

    if (!fbo_->release()) std::clog << "failed fbo_->release()\n";
}

void
SpectrographWidget::setFrozen(bool state)
{
    if (frozen_ != state) {
        frozen_ = state;
        if (!state) update();
    }
}
