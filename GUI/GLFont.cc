#include <cmath>

#include "QtCore/QStringList"
#include "QtCore/QSysInfo"
#include "QtGui/QImage"
#include "QtGui/QPainter"
#include "QtGui/QPixmap"
#include "QtOpenGL/QGLWidget"

#include "GUI/LogUtils.h"

#include "GLFont.h"

using namespace SideCar::GUI;

int GLFont::textureType_ = -1;
bool GLFont::powerOfTwoRequired_ = true;

Logger::Log&
GLFont::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.GLFont");
    return log_;
}

void
GLFont::DetermineTextureType()
{
    Logger::ProcLog log("DetermineTextureType", Log());
    LOGERROR << std::endl;

    // Basic goal is to use a non-power of two texture type since it will save texture memory and improve
    // performance. First, check OpenGL version. If version 2.0 or higher than 2D textures are not constrained
    // to power of two dimensions so use them.
    //
    QString versionText(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    LOGERROR << "versionText: " << versionText << std::endl;

    bool ok = true;
    float version = versionText.section(' ', 0, 0).toFloat(&ok);
    LOGERROR << "version: " << version << std::endl;
    if (ok && version > 2.0) {
        LOGERROR << "using GL_TEXTURE_2D (non-power-of-2)" << std::endl;
        textureType_ = GL_TEXTURE_2D;
        powerOfTwoRequired_ = false;
        return;
    }

    QString extensionsText(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
    LOGERROR << "extensions: " << extensionsText << std::endl;

    QStringList extensions = extensionsText.split(' ');

#ifdef GL_TEXTURE_RECTANGLE_ARB
    if (extensions.contains("GL_ARB_texture_rectangle")) {
        LOGERROR << "usng GL_TEXTURE_RECTANGLE_ARB" << std::endl;
        textureType_ = GL_TEXTURE_RECTANGLE_ARB;
        powerOfTwoRequired_ = false;
        return;
    }
#endif

#ifdef GL_TEXTURE_RECTANGLE_EXT
    if (extensions.contains("GL_EXT_texture_rectangle")) {
        LOGERROR << "usng GL_TEXTURE_RECTANGLE_EXT" << std::endl;
        textureType_ = GL_TEXTURE_RECTANGLE_EXT;
        powerOfTwoRequired_ = false;
        return;
    }
#endif

    LOGERROR << "usng GL_TEXTURE_2D (power-of-2)" << std::endl;
    textureType_ = GL_TEXTURE_2D;
    powerOfTwoRequired_ = true;
}

GLFont::GLFont(const QFont& font) : characterRenderInfos_(), font_(font), fontMetrics_(font)
{
    if (textureType_ == -1) DetermineTextureType();
}

GLFont::~GLFont()
{
    // Release allocated OpenGL resources.
    //
    foreach (RenderInfo info, characterRenderInfos_) {
        glDeleteTextures(1, &info.texture_);
        glDeleteLists(info.displayList_, 1);
    }
}

int
GLFont::width(const QString& text) const
{
    int w = 0;
    foreach (QChar character, text)
        w += fontMetrics_.width(character);
    return w;
}

void
GLFont::render(const VertexColorTagArray& tags)
{
    static Logger::ProcLog log("render", Log());

    // Prepare to draw the character textures. NOTE: expects the position values in the given
    // VertexColorTagArray to be in window coordinates, and it calls glLoadIdentity() in the current matrix mode
    // prior to moving to the tag position.
    //
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(textureType_);
    glEnable(GL_BLEND);

    // If this is enabled, then overlapping tags don't look right.
    //
    glDisable(GL_STENCIL_TEST);

    for (size_t index = 0; index < tags.size(); ++index) {
        const VertexColorTag& tag(tags[index]);

        LOGDEBUG << "xy: " << tag.vertex.x << ' ' << tag.vertex.y << " tag: " << tag.tag << std::endl;

        // Check to see that all of the characters in the given text string have OpenGL textures for rendering.
        //
        analyzeText(tag.tag);

        // Move to the location of the first character, and begin drawing them. The display list for each
        // character ends with a glTranslatef call that moves to the next drawing location.
        //
        glPushMatrix();
        glTranslatef(tag.vertex.x, tag.vertex.y, 0.0);

        // Set the vertex color to the given tag color.
        //
        tag.color.use();

        // Apply the character textures that make up the tag.
        //
        foreach (QChar character, tag.tag) {
            CharacterRenderInfoHash::const_iterator pos = characterRenderInfos_.find(character);
            if (pos != characterRenderInfos_.end()) {
                glCallList(pos.value().displayList_);
            } else {
                LOGERROR << "no render info for character '" << QString(character) << "'" << std::endl;
            }
        }

        glPopMatrix();
    }

    // Restore the saved OpenGL environment.
    //
    glPopAttrib();
}

void
GLFont::analyzeText(const QString& text)
{
    static Logger::ProcLog log("analyzeText", Log());

    // Visit each character in the string.
    //
    foreach (QChar character, text) {
        // See if the character already has texture data for it
        //
        CharacterRenderInfoHash::const_iterator pos = characterRenderInfos_.find(character);
        if (pos != characterRenderInfos_.end()) continue;

        LOGDEBUG << "generating texture for character '" << QString(character) << "'" << std::endl;

        // Calculate character dimensions
        //
        int width = fontMetrics_.width(character);
        int lb = fontMetrics_.leftBearing(character);
        int rb = fontMetrics_.rightBearing(character);
        if (lb < 0) width -= lb;
        if (rb < 0) width -= rb;
        if (width <= 0) continue;

        int height = fontMetrics_.height();
        LOGDEBUG << "width: " << width << " height: " << height << std::endl;

        // Create a QPixmap to draw into. Draw with white so that when the texture is used, it will use all of
        // the glColor4f() value.
        //
        QPixmap pixmap(width, height);
        pixmap.fill(Qt::transparent);

        QPainter painter;
        painter.begin(&pixmap);
        painter.setFont(font_);
        painter.setPen(Qt::white);

        int x = 0;
        if (lb < 0) x -= lb;

        painter.drawText(x, fontMetrics_.ascent(), character);
        painter.end();

        // Convert pixmap to an image to use as the basis for the texture.
        //
        QImage image = pixmap.toImage();
        if (image.depth() != 32) image = image.convertToFormat(QImage::Format_ARGB32);

        // Construct padded image to hold the texture data.
        //
        int paddedWidth = width;
        int paddedHeight = height;
        if (powerOfTwoRequired_) {
            paddedWidth = int(::pow(2, ::ceil(::log2(paddedWidth))));
            paddedHeight = int(::pow(2, ::ceil(::log2(paddedHeight))));
        }

        LOGDEBUG << "paddedWidth: " << paddedWidth << " paddedHeight: " << paddedHeight << std::endl;

        QImage paddedImage = QImage(paddedWidth, paddedHeight, QImage::Format_ARGB32);
        paddedImage.fill(Qt::transparent);

        // Copy over the bytes into the padded QImage.
        //
        if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
            LOGERROR << "big-endian" << std::endl;
            for (int y = 0; y < image.height(); ++y) {
                uint32_t* imageRgb = (uint32_t*)image.scanLine(y);
                // Mirror y-coordinates for OpenGL
                uint32_t* paddedImageRgb = (uint32_t*)paddedImage.scanLine(paddedHeight - 1 - y);
                uint32_t* end = imageRgb + image.width();
                while (imageRgb < end) {
                    // Use color channels for alpha and set all color channels to white (Qt) ARGB -> (OpenGL)
                    // RGBA
                    //
                    *paddedImageRgb++ = (*imageRgb++ & 0xFF) | 0xFFFFFF00;
                }
            }
        } else {
            LOGERROR << "little-endian" << std::endl;
            for (int y = 0; y < image.height(); ++y) {
                uint32_t* imageRgb = (uint32_t*)image.scanLine(y);
                // Mirror y-coordinates for OpenGL
                uint32_t* paddedImageRgb = (uint32_t*)paddedImage.scanLine(paddedHeight - 1 - y);
                uint32_t* end = imageRgb + image.width();
                while (imageRgb < end) {
                    // Use color channels for alpha and set all color channels to white (Qt) ARGB -> (OpenGL)
                    // ABGR
                    //
                    *paddedImageRgb++ = ((*imageRgb++ << 24) & 0xFF000000) | 0x00FFFFFF;
                }
            }
        }

        // Create new RenderInfo entry for the character.
        //
        RenderInfo renderInfo;
        glGenTextures(1, &renderInfo.texture_);
        renderInfo.displayList_ = glGenLists(1);
        characterRenderInfos_[character] = renderInfo;

        // Create the OpenGL texture.
        //
        glBindTexture(textureType_, renderInfo.texture_);
        glTexParameteri(textureType_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(textureType_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(textureType_, 0, GL_ALPHA, paddedImage.width(), paddedImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     paddedImage.bits());

        // Calculate the proper positions for rendering the texture.
        //
        GLfloat minX(0), minY(0), maxX(0), maxY(0);
        switch (textureType_) {
        case GL_TEXTURE_2D:
            minX = 0;
            maxX = float(width) / paddedWidth;
            minY = 1 - float(height) / paddedHeight;
            maxY = 1;
            break;

        case GL_TEXTURE_RECTANGLE_ARB:
            minX = 0;
            minY = 0;
            maxX = width;
            maxY = height;
            break;

        default: break;
        }

        LOGERROR << "minX: " << minX << " minY: " << minY << " maxX: " << maxX << " maxY: " << maxY << std::endl;

        // Create the display list for using the texture
        //
        glNewList(renderInfo.displayList_, GL_COMPILE);
        {
            if (lb < 0) glTranslatef(lb, 0, 0);

            glBindTexture(textureType_, renderInfo.texture_);
            glBegin(GL_QUADS);
            {
                glTexCoord2f(minX, minY);
                glVertex2f(0, 0);
                glTexCoord2f(maxX, minY);
                glVertex2f(width, 0);
                glTexCoord2f(maxX, maxY);
                glVertex2f(width, height);
                glTexCoord2f(minX, maxY);
                glVertex2f(0, height);
            }
            glEnd();

            // Shift the horizontal position so that OpenGL is ready for the next character.
            //
            width = fontMetrics_.width(character);
            if (lb < 0) width -= lb;
            glTranslatef(width, 0, 0);
        }
        glEndList();
    }
}
