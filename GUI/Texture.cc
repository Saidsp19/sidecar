#include <cmath>

#include "GUI/LogUtils.h"

#include "Texture.h"

using namespace SideCar::GUI;

GLenum Texture::bestTextureType_ = GLenum(-1);
bool Texture::isPowerOfTwoSizeRequired_ = true;

Logger::Log&
Texture::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.Texture");
    return log_;
}

GLenum
Texture::GetBestTextureType()
{
    Logger::ProcLog log("GetBestTextureType", Log());

    if (int(bestTextureType_) != -1) {
	LOGINFO << GetTextureTypeTag(bestTextureType_) << std::endl;
	return bestTextureType_;
    }

    // Basic goal is to use a non-power of two texture type since it will save texture memory and improve
    // performance. First, check OpenGL version. If version 2.0 or higher than 2D textures are not constrained
    // to power of two dimensions so use them.
    //
    QString versionText(
	reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    LOGDEBUG << "versionText: " << versionText << std::endl;

    bool ok = true;
    float version = versionText.section(' ', 0, 0).toFloat(&ok);
    LOGDEBUG << "version: " << version << std::endl;
    if (ok && version > 2.0) {
	LOGDEBUG << "using GL_TEXTURE_2D (non-power-of-2)" << std::endl;
	bestTextureType_ = GL_TEXTURE_2D;
	isPowerOfTwoSizeRequired_ = false;
	return bestTextureType_;
    }

    QString extensionsText(
	reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
    LOGDEBUG << "extensions: " << extensionsText << std::endl;
    QStringList extensions = extensionsText.split(' ');

#ifdef GL_TEXTURE_RECTANGLE_ARB
    if (extensions.contains("GL_ARB_texture_rectangle")) {
	LOGDEBUG << "usng GL_TEXTURE_RECTANGLE_ARB" << std::endl;
	bestTextureType_ = GL_TEXTURE_RECTANGLE_ARB;
	isPowerOfTwoSizeRequired_ = false;
	return bestTextureType_;
    }
#endif

#ifdef GL_TEXTURE_RECTANGLE_EXT
    if (extensions.contains("GL_EXT_texture_rectangle")) {
	LOGDEBUG << "usng GL_TEXTURE_RECTANGLE_EXT" << std::endl;
	bestTextureType_ = GL_TEXTURE_RECTANGLE_EXT;
	isPowerOfTwoSizeRequired_ = false;
	return bestTextureType_;
    }
#endif

    LOGDEBUG << "usng GL_TEXTURE_2D (power-of-2)" << std::endl;
    bestTextureType_ = GL_TEXTURE_2D;
    isPowerOfTwoSizeRequired_ = true;

    return bestTextureType_;
}

bool
Texture::IsPowerOfTwoSizeRequired()
{
    GetBestTextureType();
    return isPowerOfTwoSizeRequired_;
}

GLuint
Texture::GetBestTextureSpan(GLuint span)
{
    if (IsPowerOfTwoSizeRequired())
	return GetPowerOf2Span(span);
    return span;
}

GLuint
Texture::GetPowerOf2Span(GLuint span)
{
    return GLuint(::pow(2, ::ceil(::log2(span))));
}

QString
Texture::GetTextureTypeTag(GLenum type)
{
    switch (type) {
    case GL_TEXTURE_2D: return "GL_TEXTURE_2D";
#ifdef GL_TEXTURE_RECTANGLE_ARB
    case GL_TEXTURE_RECTANGLE_ARB: return "GL_TEXTURE_RECTANGLE_ARB";
#else
#ifdef GL_TEXTURE_RECTANGLE_EXT
    case GL_TEXTURE_RECTANGLE_EXT: return "GL_TEXTURE_RECTANGLE_EXT";
#endif
#endif
    default: return "UNKNOWN";
    }
}

Texture::Texture(GLenum type, GLuint id, GLuint width, GLuint height)
    : type_(type), id_(id), width_(width), height_(height),
      xMin_(0.0), yMin_(0.0)
{
    Logger::ProcLog log("Texture", Log());
    LOGINFO << "type: " << GetTextureTypeTag(type) << " id: " << id
	    << " width: " << width << " height: " << height << std::endl;

    if (isPowerOfTwoSizeRequired_) {
	width = GetBestTextureSpan(width);
	height = GetBestTextureSpan(height);
	LOGDEBUG << "powerOf2 width: " << width << " height: " << height
		 << std::endl;
    }

    switch (type) {
    case GL_TEXTURE_2D:
	xMax_ = float(width_) / float(width);
	yMax_ = float(height_) / float(height);
	break;

#ifdef GL_TEXTURE_RECTANGLE_ARB
    case GL_TEXTURE_RECTANGLE_ARB:
#else
#ifdef GL_TEXTURE_RECTANGLE_EXT
    case GL_TEXTURE_RECTANGLE_EXT:
#endif
#endif
	xMax_ = width_;
	yMax_ = height_;
	break;
    }

    LOGDEBUG << "xMax: " << xMax_ << " yMax: " << yMax_ << std::endl;
}

void
Texture::setBounds(GLfloat xMin, GLfloat yMin, GLfloat xMax, GLfloat yMax)
{
    xMin_ = xMin;
    yMin_ = yMin;
    xMax_ = xMax;
    yMax_ = yMax;
}
