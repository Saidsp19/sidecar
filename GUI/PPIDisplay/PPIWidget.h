#ifndef SIDECAR_GUI_PPIDISPLAY_PPIWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_PPIWIDGET_H

#include "QtCore/QBasicTimer"
#include "QtCore/QList"
#include "QtOpenGL/QGLWidget"

#ifdef darwin
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

class QImage;
class QSettings;

#include "GUI/Texture.h"
#include "GUI/Utils.h"

#include "History.h"

namespace Logger {
class Log;
}
namespace Utils {
class SineCosineLUT;
}

namespace SideCar {
namespace GUI {

class BugPlotEmitterSettings;
class ChannelImaging;
class MessageList;
class PhantomCursorImaging;
class RangeMap;
class RangeRingsImaging;
class RangeTruthsImaging;
class SampleImaging;
class TargetPlotImaging;

namespace PPIDisplay {

class BackgroundImageSettings;
class BinaryVertexGenerator;
class Configuration;
class CursorPosition;
class DecaySettings;
class MagnifierWindow;
class OffscreenBuffer;
class VideoImaging;
class VideoVertexGenerator;
class ViewSettings;

/** Derivation of a QGLWidget widget that acts like a simple PPI display. Incoming data arrives via the
    processVideo(), processBinary(), and processExtractions() slots. Each slot takes a reference to a
    MessageBlockList object which contains one or more ACE_Message_Block objects created by a separate reader
    thread. The incoming slot obtains a message object from the raw ACE_Message_Block, adds it to the History
    object, and if necessary, generates data for future display.

    The Video and BinaryVideo processing paths render into an OffscreenBuffer object, using an OpenGL pixel buffer or a
    framebuffer object to do the dirty work. The paintGL() method of the PPIWidget uses the offscreen renderings to
    create the PPI display. Extraction reports exist as a list of points, which the paintGL() method renders directly
    to the main view.

    This class supports the loading of an image file, which gets blended with the PPI image. The image has an
    associated opacity value (as do all of the other rendering images), which may be changed through the application's
    ConfigurationWindow object. For best results, the image file should be square, centered at the radar's center, and
    with a dimension 2 * rangeMax, the maximum range of the incoming radar reports.

    <h2>Viewport Control</h2>

    The user may manipulate the view by panning or zooming. Panning is accomplished by clicking the left mouse button
    while holding the SHIFT key down and then moving the mouse, or by using the arrow keys of the keyboard. The zoom
    level is controlled via the mouse scroll wheel, or by the letter keys 'I' and 'O' for zooming in and out.

    <h2>Magnifiers</h2>

    Instead of zooming the entire display, one can create a separate view window that only shows a subsection of the
    main view. To create a new magnification view, click the left mouse button down and then move the mouse to create a
    rectangle on the screen. When the left mouse button is released, a new Magnifier window object will pop up, showing
    the area circumscribed by the rectangle on the screen.
*/
class PPIWidget : public QGLWidget {
    Q_OBJECT
    using Super = QGLWidget;

public:
    /** Obtain the Log device to use for objects of this classs.

        \return Loggger::Log reference
    */
    static Logger::Log& Log();

    /** Obtain the OpenGL configuration we want to use.

        \return OpenGL format
    */
    static QGLFormat GetGLFormat();

    /** Constructor. Do some construction here, but most of the OpenGL initialization is performed in the

        initializeGL() method. \param parent parent object for automatic destruction
    */
    PPIWidget(QWidget* parent = 0);

    /** Destructor. Clean up OpenGL objects not handled by Qt.
     */
    ~PPIWidget();

    /** Convert local (mouse) coordinates into real-world values.

        \param inX local X value

        \param inY local Y value

        \param outX real-world X value output

        \param outY real-world Y value output
    */
    void localToRealWorld(int inX, int inY, GLdouble& outX, GLdouble& outY) const;

    void updateCursorPosition(CursorPosition& pos);

    /** Pan the view by fractions of the existing viewport dimensions. For instance, pan(0.5, 0.25) will pan
        horizontally by 1/2 of the width, and 1/4 of the height of the viewport. Positive values move right and
        up, whereas negative values move left and down.

        \param xf horizontal fraction to pan

        \param yf vertical fraction to pan
    */
    void pan(double xf, double yf);

    /** Change the zoom level for the viewport. Zoom is represented as a zoom factor raised by a zoom power.
        This method affects the zoom power.

        \param change amount to change the zoom power
    */
    void changeZoom(int change);

    /** Initialize a view context, either ourselves or that of a Magnifier object.

        \param modelViewMatrix storage for the model view matrix
    */
    void initializeContext(GLdouble* modelViewMatrix);

    /** Render a display into the given widget.

        \param widget where to render
    */
    void renderScene(QGLWidget* widget);

    /** Obtain the visibility of the phantom cursor.

        \return true if the phantom cursor is visible in the view
    */
    bool getShowPhantomCursor() const { return showPhantomCursor_; }

    /** Obtain the visibility of the cursor position tooltip.

        \return true if the tooltip is shown in the view
    */
    bool getShowCursorPosition() const { return showCursorPosition_; }

    /** Set the settings group key for us to use later when we must restore settings from a QSettings object.

        \param settingsKey group key to apply before accessing setting values
    */
    void setSettingsKey(const QString& settingsKey) { settingsKey_ = settingsKey; }

signals:

    /** Notification of the latest incomin message received.

        \param msg the last message
    */
    void currentMessage(Messages::PRIMessage::Ref msg);

    /** Notification of the current mouse position in real-world coordinates.

        \param pos real-world position
    */
    void currentCursorPosition(const QPointF& pos);

public slots:

    /** Associate a new magnifier window with this view.

        \param magnifier MagnifierWindow object to associate
    */
    void addMagnifier(MagnifierWindow* magnifier);

    /** Remove a magnifier window from this view.

        \param magnifier MagnifierWindow object to remove
    */
    void removeMagnifier(MagnifierWindow* magnifier = 0);

    /** Save associated magnifier view settings.

        \param settings QSettings object to write into
    */
    void saveMagnifiers(QSettings& settings) const;

    /** Make sure all magnifier windows hover above our window.
     */
    void raiseMagnifiers();

    /** Obtain the real-world coordinates of the current mouse cursor position, and then pan the viewport so
        that the coordinates are found at the center of the windoow. Contrary to human factors orthodoxy, we
        also move the mouse pointer to the center of the window.
    */
    void centerAtCursor();

    /** Clear all of the layers: video, binary, extractions, bug plots, and TSPI plots.
     */
    void clearAll();

    /** Clear the video data layer.
     */
    void clearVideoBuffer();

    /** Clear the binary data layer.
     */
    void clearBinaryBuffer();

    /** Clear the extractions plot layer.
     */
    void clearExtractions();

    /** Clear the TSPI plot layer.
     */
    void clearRangeTruths();

    /** Clear the user bug plot layer.
     */
    void clearBugPlots();

    /** Repaint all of the video data for the current rotation
     */
    void redisplayVideo();

    /** Repaint all of the binary video data for the current rotation
     */
    void redisplayBinary();

    /** Process one or more Video messages.

        \param data container of Video message references
    */
    void processVideo(const MessageList& data);

    /** Process one or more BinaryVideo messages.

        \param data container of BinarVideo message references
    */
    void processBinary(const MessageList& data);

    /** Process one or more target Extractions messages.

        \param data container of Extractions message references
    */
    void processExtractions(const MessageList& data);

    /** Process one or more TSPI messages.

        \param data container of TSPI message references
    */
    void processRangeTruths(const MessageList& data);

    /** Process one or more user plot messages.

        \param data container of user plot message references
    */
    void processBugPlots(const MessageList& data);

    /** Set the location of the phantom cursor.

        \param pos location of the phantom cursor in real-world coordinates
    */
    void setPhantomCursor(const QPointF& pos);

    /** Change the visibility state of the phantom cursor.

        \param state true if the phantom cursor is shown
    */
    void showPhantomCursor(bool state);

    void setCursorPosition(const QString& value);

    void setShowCursorPosition(bool state);

private slots:

    /** Notification that the video channel subscription changed.

        \param name new channel
    */
    void videoChannelChanged(const QString& name);

    /** Notification that the binary video channel subscription changed.

        \param name new channel
    */
    void binaryChannelChanged(const QString& name);

    /** Notification that the target extractions channel subscription changed.

        \param name new channel
    */
    void extractionsChannelChanged(const QString& name);

    /** Notification that the TSPI channel subscription changed.

        \param name new channel
    */
    void rangeTruthsChannelChanged(const QString& name);

    /** Notification that the user plot channel subscription changed.

        \param name new channel
    */
    void bugPlotsChannelChanged(const QString& name);

    /** Notification that the range max value changed.

        \param value new value
    */
    void rangeMaxChanged(double value);

    /** Notification that the range max max value changed.

        \param value new value
    */
    void rangeMaxMaxChanged(double value);

    /** Remake the simulated phosphor decay mask texture.
     */
    void remakeDecayTexture();

    /** Remake the desaturation decay mask texture, used when using a color map with saturated values at the
        high end of the map.
    */
    void remakeDesaturationTexture();

    /** Remake the range map lines.
     */
    void remakeRangeMap();

    /** Remake the range/azimuth rings.
     */
    void remakeRangeRings();

    /** Make a new OffscreenBuffer object for video data
     */
    void makeVideoBuffer();

    /** Dispose of the OffscreenBuffer object used for video data.
     */
    void deleteVideoBuffer();

    /** Make a new OffscreenBuffer object for binary video data
     */
    void makeBinaryBuffer();

    /** Dispose of the OffscreenBuffer object used for binary video data.
     */
    void deleteBinaryBuffer();

    /** Update viewport when a view setting changes due to panning or zooming.
     */
    void viewSettingsChanged();

    /** Create a background texture based off of an image.

        \param image the image to use for the texture.
    */
    void makeBackgroundTexture(const QImage& image);

    /** Notification handler called when the view changes to show a different historical point in time.

        \param age the past in terms of number of revolutions
    */
    void historyCurrentViewChanged(int age);

    /** Notification handler called when the current history view gets older by one revolution.

        \param age how old the view is in number of revolutions
    */
    void historyCurrentViewAged(int age);

private:
    /** Indices for OpenGL display lists.
     */
    enum ListIndex {
        kRenderVideoTexture = 0,
        kRenderBinaryTexture,
        kRenderBackgroundTexture,
        kDesaturation,
        kDecay,
        kRangeMap,
        kRangeRings,
        kBeginRender,
        kNumLists
    };

    /** Convert a ListIndex into an OpenGL display list identifier.

        \param index value to convert

        \return OpenGL display list identifier
    */
    GLuint getDisplayList(ListIndex index) const { return displayLists_ + index; }

    /** Initialize the OpenGL environment.
     */
    void initializeGL();
    void restoreMagnifiers();
    void makeRenderTextureList(ListIndex index, const Texture& texture);
    void makeDecayTexture();
    void makeDecayList();
    void makeDesaturationTexture();
    void makeDesaturationList();
    void makeRangeMapList();
    void makeRangeRingsList();
    void showCursorInfo();
    void setViewTransform();

    /** Resize the GL view. Enforces that the viewport is square.

        \param w width of the resized size

        \param h height of the resized size
    */
    void resizeGL(int width, int height);

    /** Update the GL display with the lates PPI information.
     */
    void paintGL();

    void enterEvent(QEvent* event);

    void mousePressEvent(QMouseEvent* event);

    void mouseMoveEvent(QMouseEvent* event);

    void mouseReleaseEvent(QMouseEvent* event);

    void wheelEvent(QWheelEvent* event);

    void keyPressEvent(QKeyEvent* event);

    void keyReleaseEvent(QKeyEvent* event);

    void timerEvent(QTimerEvent* event);

    void showEvent(QShowEvent* event);

    void closeEvent(QCloseEvent* event);

    void leaveEvent(QEvent* event);

    void repaintVideo(const History::MessageVector& video);

    void repaintBinary(const History::MessageVector& binary);

    void drawExtractions(QWidget* widget);

    void drawRangeTruths(QWidget* widget);

    void drawBugPlots(QWidget* widget);

    void showMessage(const QString& text, int duration = 5000) const;

    void buildDiscTexture() const;

    double xSpan_;
    double ySpan_;
    double xScale_;
    double yScale_;
    double lastAzimuth_;
    uint32_t lastShaftEncoding_;

    QPoint magStartPos_;
    double magStartX_;
    double magStartY_;
    double magEndX_;
    double magEndY_;

    ViewSettings* viewSettings_;
    VideoImaging* videoImaging_;
    SampleImaging* binaryImaging_;
    TargetPlotImaging* extractionsImaging_;
    RangeTruthsImaging* rangeTruthsImaging_;
    TargetPlotImaging* bugPlotsImaging_;
    ChannelImaging* rangeMapImaging_;
    RangeRingsImaging* rangeRingsImaging_;
    BackgroundImageSettings* backgroundImageSettings_;
    DecaySettings* decaySettings_;
    PhantomCursorImaging* phantomCursorImaging_;
    BugPlotEmitterSettings* bugPlotEmitterSettings_;
    RangeMap* rangeMap_;

    History* history_;
    OffscreenBuffer* videoBuffer_;
    VideoVertexGenerator* videoVertexGenerator_;
    OffscreenBuffer* binaryBuffer_;
    BinaryVertexGenerator* binaryVertexGenerator_;
    // GLUquadricObj* textureMappingObj_;
    Utils::SineCosineLUT* sineCosineLUT_;

    GLuint displayLists_;
    Texture decayTexture_;
    Texture desaturationTexture_;
    Texture backgroundTexture_;
    GLint viewPort_[4];
    GLdouble modelViewMatrix_[16];
    GLdouble projectionMatrix_[16];

    QBasicTimer updateTimer_;

    QPointF phantomCursor_;
    QPoint panFrom_;
    QPoint mouse_;
    Messages::PRIMessage::Ref info_;
    QString settingsKey_;
    QList<MagnifierWindow*> magnifiers_;
    QString cursorPosition_;

    bool panning_;
    bool rubberBanding_;
    bool showPhantomCursor_;
    bool showCursorPosition_;
    bool trimVideo_;
    bool trimBinary_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
