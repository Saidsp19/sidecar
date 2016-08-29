#ifndef SIDECAR_GUI_BSCOPE_PPIWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_PPIWIDGET_H

#include "QtCore/QBasicTimer"
#include "QtCore/QList"
#include "QtOpenGL/QGLWidget"

class QImage;
class QSettings;

#include "GUI/Texture.h"
#include "GUI/Utils.h"

#include "History.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class BugPlotEmitterSettings;
class ChannelImaging;
class CursorPosition;
class MessageList;
class PhantomCursorImaging;
class RangeRingsImaging;
class SampleImaging;
class TargetPlotImaging;
class VideoSampleCountTransform;

namespace BScope {

class BinaryVertexGenerator;
class Configuration;
class HistorySettings;
class MagnifierWindow;
class OffscreenBuffer;
class RangeMap;
class VideoImaging;
class VideoVertexGenerator;
class ViewSettings;

/** Derivation of an OpenGL widget that acts like a simple PPI display. Incoming data arrives via the
    processVideo(), processBinary(), and processExtractions() slots. Each slot takes a reference to a
    MessageBlockList object which contains one or more ACE_Message_Block objects created in a separate reader
    thread. The incoming slot obtains a message object from the raw ACE_Message_Block, adds it to the History
    object, and if necessary, generates data for future display.

    The Video and BinaryVideo processing paths render into an OffscreenBuffer
    object, using an OpenGL pixel buffer or a framebuffer object to do the
    dirty work. The paintGL() method of the PPIWidget uses the offscreen
    renderings to create the PPI display. Extraction reports exist as a list of
    points, which the paintGL() method renders directly to the main view.

    This class supports the loading of an image file, which gets blended with
    the PPI image. The image has an associated opacity value (as do all of the
    other rendering images), which may be changed through the application's
    ConfigurationWindow object. For best results, the image file should be
    square, centered at the radar's center, and with a dimension 2 * rangeMax,
    the maximum range of the incoming radar reports.

    The user may manipulate the view by panning or zooming. Currently
*/
class PPIWidget :  public QGLWidget
{
    Q_OBJECT
    using Super = QGLWidget;
public:

    enum RenderType {
	kMainWindow,
	kMagnifierWindow,
	kCaptureBuffer
    };
    
    static Logger::Log& Log();

    /** Obtain the OpenGL configuration we want to use.

        \return OpenGL format
    */
    static QGLFormat GetGLFormat();

    /** Constructor.

        \param parent parent object
    */
    PPIWidget(QWidget* parent = 0);

    ~PPIWidget();

    void localToRealWorld(int x, int y, GLdouble& azimuth, GLdouble& range)
	const;

    void updateCursorPosition(CursorPosition& pos);

    void initializeContext();

    void renderScene(QGLWidget* widget, RenderType renderType);

    bool getShowPhantomCursor() const { return showPhantomCursor_; }

    bool getShowCursorPosition() const { return showCursorPosition_; }

    void setSettingsKey(const QString& settingsKey)
	{ settingsKey_ = settingsKey; }

signals:

    void currentMessage(Messages::PRIMessage::Ref msg);

    void currentCursorPosition(const QPointF& pos);

    void transformChanged();

public slots:

    void addMagnifier(MagnifierWindow* magnifier);

    void removeMagnifier(MagnifierWindow* magnifier = 0);

    void saveMagnifiers(QSettings& settings) const;
    
    void raiseMagnifiers();
    
    void clearAll();

    void clearVideoBuffer();

    void clearBinaryBuffer();

    void clearExtractions();

    void clearRangeTruths();

    void clearBugPlots();

    void redisplayAll();

    void redisplayVideo();

    void redisplayBinary();

    void processVideo(const MessageList& data);

    void processBinary(const MessageList& data);

    void processExtractions(const MessageList& data);

    void processRangeTruths(const MessageList& data);

    void processBugPlots(const MessageList& data);

    void showPhantomCursor(bool state);

    void setPhantomCursor(const QPointF& pos);

    void setCursorPosition(const QString& value);

    void setShowCursorPosition(bool state);

private slots:

    void viewSettingsChanged();

    void videoChannelChanged(int index);

    void binaryChannelChanged(int index);

    void extractionsChannelChanged(int index);

    void rangeTruthsChannelChanged(int index);

    void bugPlotsChannelChanged(int index);

    void remakeRangeMap();

    void remakeRangeRings();

    void makeVideoBuffer();

    void deleteVideoBuffer();

    void makeBinaryBuffer();

    void deleteBinaryBuffer();

private:

    enum ListIndex {
	kRenderVideoTexture = 0,
	kRenderBinaryTexture,
	kRangeMap,
	kRangeRings,
	kBeginRender,
	kNumLists
    };

    GLuint getDisplayList(ListIndex index) const
	{ return displayLists_ + index; }

    /** Initialize the OpenGL environment.
     */
    void initializeGL();
    void restoreMagnifiers();
    void makeRenderTextureList(ListIndex index, const Texture& texture);
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

    void mousePressEvent(QMouseEvent* event);

    void mouseMoveEvent(QMouseEvent* event);

    void mouseReleaseEvent(QMouseEvent* event);

    void timerEvent(QTimerEvent* event);

    void showEvent(QShowEvent* event);

    void closeEvent(QCloseEvent* event);

    void leaveEvent(QEvent* event);

    void updateTransform();

    void repaintVideo(const History::MessageVector& video);

    void repaintBinary(const History::MessageVector& binary);

    void drawExtractions(QWidget* widget);

    void drawRangeTruths(QWidget* widget);

    void drawBugPlots(QWidget* widget);

    void renderTexture(GLuint texture);

    double lastAzimuth_;
    double xScale_;
    double yScale_;
    double azimuthScaling_;

    QPoint magStartPos_;
    double magStartX_;
    double magStartY_;
    double magEndX_;
    double magEndY_;

    ViewSettings* viewSettings_;
    HistorySettings* historySettings_;
    VideoImaging* videoImaging_;
    SampleImaging* binaryImaging_;
    TargetPlotImaging* extractionsImaging_;
    TargetPlotImaging* rangeTruthsImaging_;
    TargetPlotImaging* bugPlotsImaging_;
    ChannelImaging* rangeMapImaging_;
    RangeRingsImaging* rangeRingsImaging_;
    PhantomCursorImaging* phantomCursorImaging_;
    BugPlotEmitterSettings* bugPlotEmitterSettings_;
    RangeMap* rangeMap_;

    History* history_;
    OffscreenBuffer* videoBuffer_;
    VideoVertexGenerator* videoVertexGenerator_;
    OffscreenBuffer* binaryBuffer_;
    BinaryVertexGenerator* binaryVertexGenerator_;

    GLuint displayLists_;
    QBasicTimer updateTimer_;
    QPoint mouse_;
    QPointF phantomCursor_;
    Messages::PRIMessage::Ref info_;
    QString settingsKey_;
    QList<MagnifierWindow*> magnifiers_;
    QString cursorPosition_;

    bool rubberBanding_;
    bool showPhantomCursor_;
    bool showCursorPosition_;
    bool trimVideo_;
    bool trimBinary_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
