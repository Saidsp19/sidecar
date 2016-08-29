#ifndef SIDECAR_GUI_OFFSCREENBUFFER_H // -*- C++ -*-
#define SIDECAR_GUI_OFFSCREENBUFFER_H

#include "GUI/Texture.h"
#include "GUI/VertexColorArray.h"
#include "Messages/PRIMessage.h"

class QGLFramebufferObject;

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class SampleImaging;

/** Manager of an OpenGL framebuffer object that allows for direct rendering into a texture object.

    Uses an QGLFramebufferObject to do the OpenGL dirty work.
*/
class OffscreenBuffer : public QObject
{
    Q_OBJECT
public:

    /** Obtain Log device to use for OffscreenBuffer messages

        \return Log device
    */
    static Logger::Log& Log();

    /** Constructor. Creates a new OpenGL framebuffer object and initializes it to render into a new 2D texture
        object.

        \param imaging collection of settings that control how imaging is
        perfomed
    */
    OffscreenBuffer(const SampleImaging* imaging, double xMin, double xMax,
                    double yMin, double yMax, int width, int height,
                    int textureType = -1);

    /** Destructor. Destroys the created OpenGL framebuffer and 2D texture objects.
     */
    ~OffscreenBuffer();

    /** Determine if the QGLFramebufferObject is valid and ready to use.

        \return true if valid, false if the texture size is too big or there is
        no framebuffer support.
    */
    bool isValid() const;

    bool isInitialized() const { return displayLists_ != 0; }

    /** Obtain the ID of the render texture. NOTE: use of this by another thread is not supported by OpenGL.

        \return texture ID
    */
    const Texture getTexture() const { return texture_; }

    /** Obtain the imaging configuration for the rendering process.

        \return read-only reference
    */
    const SampleImaging* getImaging() const { return imaging_; }

    /** Prepare the OpenGL context to render the point data added by previous addPoint() calls. Invokes
        glDrawArrays() to perform the rendering.
    */
    void renderPoints(const VertexColorArray& points);

    void setBounds(double xMin, double xMax, double yMin, double yMax);

    double getXMin() const { return xMin_; }

    double getXMax() const { return xMax_; }

    double getYMin() const { return yMin_; }

    double getYMax() const { return yMax_; }

    bool contains(double x, double y) const
	{ return x >= getXMin() && x <= getXMax() &&
		y >= getYMin() && y <= getYMax(); }

    bool intersects(double x0, double y0, double x1, double y1) const;

public slots:

    /** Clear the render texture.
     */
    void clearBuffer();

    void remakeDisplayLists();

private:

    enum ListIndex {
	kBeginRender,
	kEndRender,
	kNumLists
    };

    GLuint getDisplayList(int index) const
	{ return displayLists_ + index; }

    void makeDisplayLists();

    const SampleImaging* imaging_;
    QGLFramebufferObject* fbo_;
    GLuint displayLists_;
    Texture texture_;
    double xMin_, xMax_, yMin_, yMax_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
