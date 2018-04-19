#ifndef SIDECAR_GUI_GLFONT_H // -*- C++ -*-
#define SIDECAR_GUI_GLFONT_H

#include "QtCore/QHash"
#include "QtGui/QFont"
#include "QtGui/QFontMetrics"

class QString;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class VertexColorTagArray;

class GLFont {
public:
    static Logger::Log& Log();

    GLFont(const QFont& font);

    ~GLFont();

    int width(const QString& text) const;

    void render(const VertexColorTagArray& tags);

    const QFont& getFont() const { return font_; }

    const QFontMetrics& getFontMetrics() const { return fontMetrics_; }

private:

    // create all needed textures and display lists to paint the string
    void analyzeText(const QString& text);

    struct RenderInfo {
        RenderInfo() : texture_(0), displayList_(0) {}
        unsigned int texture_;
        unsigned int displayList_;
    };

    using CharacterRenderInfoHash = QHash<QChar, RenderInfo>;
    CharacterRenderInfoHash characterRenderInfos_;
    QFont font_;
    QFontMetrics fontMetrics_;

    static void DetermineTextureType();

    static int textureType_;
    static bool powerOfTwoRequired_;
};

} // namespace GUI
} // namespace SideCar

#endif
