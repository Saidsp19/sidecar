#ifndef SIDECAR_GUI_TEXTURE_H // -*- C++ -*-
#define SIDECAR_GUI_TEXTURE_H

#include "QtOpenGL/QGLWidget"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

/** Utility class that encapsulates some attributes of an OpenGL texture. Provides a means to determine if
    non-power-of-2 (NPOT) textures are available, and if not then if some rectangular extensions are available.
*/
class Texture 
{
public:

    static Logger::Log& Log();

    static GLenum GetBestTextureType();

    static bool IsPowerOfTwoSizeRequired();

    static GLuint GetBestTextureSpan(GLuint span);

    static GLuint GetPowerOf2Span(GLuint span);

    static QString GetTextureTypeTag(GLenum type);

    Texture() : type_(GL_TEXTURE_2D), id_(0) {}

    Texture(GLenum type, GLuint id, GLuint width, GLuint height);

    GLenum getType() const { return type_; }
    GLuint getId() const { return id_; }
    GLuint getWidth() const { return width_; }
    GLuint getHeight() const { return height_; }
    
    GLfloat getXMin() const { return xMin_; }
    GLfloat getYMin() const { return yMin_; }
    GLfloat getXMax() const { return xMax_; }
    GLfloat getYMax() const { return yMax_; }

    void setBounds(GLfloat xMin, GLfloat yMin, GLfloat xMax, GLfloat yMax);
    void setXMin(GLfloat xMin) { xMin_ = xMin; }
    void setYMin(GLfloat yMin) { yMin_ = yMin; }
    void setXMax(GLfloat xMax) { xMax_ = xMax; }
    void setYMax(GLfloat yMax) { yMax_ = yMax; }

    bool isValid() const { return id_ != 0; }
    void invalidate() { id_ = 0; }
    operator bool() const { return id_ != 0; }

private:
    GLenum type_;
    GLuint id_;
    GLuint width_;
    GLuint height_;
    GLfloat xMin_;
    GLfloat yMin_;
    GLfloat xMax_;
    GLfloat yMax_;

    static GLenum bestTextureType_;
    static bool isPowerOfTwoSizeRequired_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
