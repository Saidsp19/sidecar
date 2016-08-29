#ifndef SIDECAR_GUI_SPECTRUM_SPECTROGRAPHWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_SPECTROGRAPHWIDGET_H

#include "QtCore/QBasicTimer"
#include "QtOpenGL/QGLWidget"

#include "GUI/Texture.h"
#include "GUI/Utils.h"
#include "GUI/VertexColorArray.h"

class QGLFramebufferObject;

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class VertexColorArray;

namespace Spectrum {

class Configuration;
class FFTSettings;
class SpectrographImaging;

class SpectrographWidget :  public QGLWidget
{
    Q_OBJECT
    using Super = QGLWidget;
public:

    static Logger::Log& Log();

    /** Obtain the OpenGL configuration we want to use.

        \return OpenGL format
    */
    static QGLFormat GetGLFormat();

    /** Constructor.

        \param parent parent object
    */
    SpectrographWidget(QWidget* parent = 0);

    ~SpectrographWidget();

signals:

    void currentCursorPosition(const QPointF& pos);

public slots:

    void clear();

    void processBins(const QVector<QPointF>& bins);

    void needUpdate();

    void setFrozen(bool state);

private slots:

    void sizeChanged();

private:

    enum ListIndex {
	kBeginUpdate = 0,
	kBeginPaint,
	kCopyPrevious0,
	kCopyPrevious1,
	kPaintTexture0,
	kPaintTexture1,
	kNumLists
    };

    GLuint getDisplayList(int index) const
	{ return displayLists_ + index; }

    /** Initialize the OpenGL environment.
     */
    void initializeGL();

    void resizeGL(int width, int height);

    void makeOffscreenBuffer();

    void deleteOffscreenBuffer();

    void makeDisplayLists();

    /** Update the GL display with the lates PPI information.
     */
    void paintGL();

    void timerEvent(QTimerEvent* event);

    void showEvent(QShowEvent* event);

    void closeEvent(QCloseEvent* event);

    SpectrographImaging* imaging_;
    FFTSettings* fftSettings_;
    QGLFramebufferObject* fbo_;
    QSize size_;
    Texture textures_[2];
    int readTexture_;
    int writeTexture_;
    VertexColorArray points_;
    QBasicTimer updateTimer_;
    QPoint mouse_;
    GLuint displayLists_;
    bool needUpdate_;
    bool doClear_;
    bool frozen_;
};

} // End namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
