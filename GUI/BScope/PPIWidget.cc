#include "QtCore/QSettings"
#include "QtGui/QCursor"
#include "QtGui/QImage"
#include "QtGui/QMessageBox"

#include "GUI/BugPlotEmitterSettings.h"
#include "GUI/ChannelSetting.h"
#include "GUI/GLInitLock.h"
#include "GUI/LogUtils.h"
#include "GUI/MessageList.h"
#include "GUI/PhantomCursorImaging.h"
#include "GUI/RangeRingsImaging.h"
#include "GUI/StencilBufferState.h"
#include "GUI/TargetPlotImaging.h"
#include "GUI/VertexColorArray.h"
#include "GUI/VideoSampleCountTransform.h"
#include "Utils/Utils.h"

#include "App.h"
#include "BinaryVertexGenerator.h"
#include "Configuration.h"
#include "CursorPosition.h"
#include "HistorySettings.h"
#include "MagnifierWindow.h"
#include "OffscreenBuffer.h"
#include "PPIWidget.h"
#include "RangeMap.h"
#include "VideoImaging.h"
#include "VideoOffscreenBuffer.h"
#include "VideoVertexGenerator.h"
#include "ViewSettings.h"

using namespace Utils;
using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::BScope;

static int kUpdateRate = 33;	// msecs between update() calls (~30 FPS)

Logger::Log&
PPIWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.PPIWidget");
    return log_;
}

QGLFormat
PPIWidget::GetGLFormat()
{
    QGLFormat format = QGLFormat::defaultFormat();
    format.setAlpha(true);
    format.setDoubleBuffer(true);
    format.setStencil(true);
    format.setAccum(false);
    format.setDepth(false);
    format.setSampleBuffers(true);
    return format;
}

PPIWidget::PPIWidget(QWidget* parent)
    : Super(GetGLFormat(), parent),
      lastAzimuth_(-M_PI * 4), azimuthScaling_(1.0), viewSettings_(0),
      videoImaging_(0), binaryImaging_(0), extractionsImaging_(0),
      rangeTruthsImaging_(0), bugPlotsImaging_(0), rangeMapImaging_(0),
      rangeRingsImaging_(0), phantomCursorImaging_(0),
      bugPlotEmitterSettings_(0), rangeMap_(new RangeMap), history_(0),
      videoBuffer_(0), videoVertexGenerator_(new VideoVertexGenerator),
      binaryBuffer_(0), binaryVertexGenerator_(new BinaryVertexGenerator),
      displayLists_(0), updateTimer_(), mouse_(), info_(), settingsKey_(),
      magnifiers_(), cursorPosition_(), rubberBanding_(false),
      showPhantomCursor_(true), showCursorPosition_(true),
      trimVideo_(true), trimBinary_(true)
{
    Logger::ProcLog log("PPIWidget", Log());
    LOGINFO << std::endl;

    // Widget initialization. NOTE: all Open/GL initialization occurs in the initializeGL method.
    //
    setCursor(Qt::CrossCursor);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    phantomCursor_ = phantomCursorImaging_->InvalidCursor();
}

PPIWidget::~PPIWidget()
{
    deleteVideoBuffer();
    deleteBinaryBuffer();
    makeCurrent();
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    delete rangeMap_;
    delete videoVertexGenerator_;
    delete binaryVertexGenerator_;
}

void
PPIWidget::deleteVideoBuffer()
{
    if (videoBuffer_) {
	makeCurrent();
	delete videoBuffer_;
	videoBuffer_ = 0;
    }
}

void
PPIWidget::makeVideoBuffer()
{
    deleteVideoBuffer();

    makeCurrent();
    videoBuffer_ = new OffscreenBuffer(videoImaging_, viewSettings_, width(),
                                       height());
    if (! videoBuffer_->isValid())
	QMessageBox::critical(window(), "Bad Format",
                              "Failed to create offscreen texture to display "
                              "Video message data.");
    makeCurrent();
    makeRenderTextureList(kRenderVideoTexture, videoBuffer_->getTexture());
}

void
PPIWidget::deleteBinaryBuffer()
{
    if (binaryBuffer_) {
	makeCurrent();
	delete binaryBuffer_;
	binaryBuffer_ = 0;
    }
}

void
PPIWidget::makeBinaryBuffer()
{
    deleteBinaryBuffer();
    
    binaryBuffer_ = new OffscreenBuffer(binaryImaging_, viewSettings_, width(),
                                        height());
    if (! binaryBuffer_->isValid())
	QMessageBox::critical(window(), "Bad Format",
                              "Failed to create offscreen texture to display "
                              "BinaryVideo message data.");
    makeCurrent();
    makeRenderTextureList(kRenderBinaryTexture, binaryBuffer_->getTexture());
}

void
PPIWidget::makeRenderTextureList(ListIndex index, const Texture& texture)
{
    Logger::ProcLog log("makeRenderTextureList", Log());
    LOGERROR << "index: " << index << " texture: " << texture.getId()
	     << std::endl;

    double azimuthMin = viewSettings_->getAzimuthMin();
    double azimuthMax = viewSettings_->getAzimuthMax();
    double rangeMin = viewSettings_->getRangeMin();
    double rangeMax = viewSettings_->getRangeMax();

    GLEC(glNewList(getDisplayList(index), GL_COMPILE));
    {
	GLEC(glColor4f(1.0, 1.0, 1.0, 1.0));
	GLEC(glEnable(texture.getType()));
	GLEC(glBindTexture(texture.getType(), texture.getId()));
	GLEC(glBegin(GL_QUADS));
	GLEC(glTexCoord2f(texture.getXMin(), texture.getYMin()));
	GLEC(glVertex2f(azimuthMin, rangeMin));
	GLEC(glTexCoord2f(texture.getXMax(), texture.getYMin()));
	GLEC(glVertex2f(azimuthMax, rangeMin));
	GLEC(glTexCoord2f(texture.getXMax(), texture.getYMax()));
	GLEC(glVertex2f(azimuthMax, rangeMax));
	GLEC(glTexCoord2f(texture.getXMin(), texture.getYMax()));
	GLEC(glVertex2f(azimuthMin, rangeMax));
	GLEC(glEnd());
	GLEC(glBindTexture(texture.getType(), 0));
	GLEC(glDisable(texture.getType()));
    }
    GLEC(glEndList());
}

void
PPIWidget::initializeContext()
{
    // Enable the use of vertex and color arrays when calling glDrawArrays(). Additional setup happens inside
    // tthe VertexColorArray::draw() method.
    //
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDisable(GL_COLOR_SUM);

    // When blending is enabled, use the source alpha component to control how much of the existing pixel values
    // to use. For blending of video and binary textures, only let fragments that have a non-zero alpha affect
    // drawing, and overwrite binary fragments that pass the alpha test. This gives us normal video with
    // non-false binary video fragments on top when both are displayed at the same time.
    //
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GREATER, 0.0);
    glLogicOp(GL_COPY);

    // Turn on buffer multisampling for the best imaging.
    //
    glEnable(GL_MULTISAMPLE);

    // Turn on high-quality point and line smoothing.
    //
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH, GL_NICEST);

    // Stencilling is used by the range map and range rings lists so that they do not overdraw themselves, which
    // would cause unequal alpha values in their rendering. NOTE: do not enable here.
    //
    glClearStencil(1);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    // Set model view to identity matrix (it never changes).
    //
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // NOTE: don't change matrix modes after this. The rest of the code expects to be in projection mode.
    //
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

void
PPIWidget::initializeGL()
{
    Logger::ProcLog log("initializeGL", Log());
    LOGINFO << std::endl;

    if (displayLists_) {
	LOGERROR << "*** initializeGL called again ***" << std::endl;
	return;
    }
    
    // Create kNumLists Open/GL compiled display list identifiers.
    //
    displayLists_ = glGenLists(kNumLists);
    if (displayLists_ == 0) {
	Utils::Exception ex("failed to allocate OpenGL display lists");
	log.thrower(ex);
    }

    initializeContext();

    // Receive phantom cursor updates from the singleton App instance.
    //
    App* app = App::GetApp();
    connect(this, SIGNAL(currentCursorPosition(const QPointF&)),
            app, SLOT(setPhantomCursor(const QPointF&)));
    connect(app, SIGNAL(phantomCursorChanged(const QPointF&)),
            SLOT(setPhantomCursor(const QPointF&)));

    // Now that our Open/GL state is adequately setup, install the various configuration and imaging objects.
    //
    Configuration* configuration = app->getConfiguration();
    bugPlotEmitterSettings_ = configuration->getBugPlotEmitterSettings();
    
    // Connect the video channel notifications.
    //
    connect(configuration->getVideoChannel(),
            SIGNAL(incoming(const MessageList&)),
            SLOT(processVideo(const MessageList&)));
    connect(configuration->getVideoChannel(),
            SIGNAL(valueChanged(int)),
            SLOT(videoChannelChanged(int)));

    // Connect the binary video channel notifications.
    //
    connect(configuration->getBinaryChannel(),
            SIGNAL(incoming(const MessageList&)),
            SLOT(processBinary(const MessageList&)));
    connect(configuration->getBinaryChannel(),
            SIGNAL(valueChanged(int)),
            SLOT(binaryChannelChanged(int)));

    // Connect the extractions channel notifications.
    //
    connect(configuration->getExtractionsChannel(),
            SIGNAL(incoming(const MessageList&)),
            SLOT(processExtractions(const MessageList&)));
    connect(configuration->getExtractionsChannel(),
            SIGNAL(valueChanged(int)),
            SLOT(extractionsChannelChanged(int)));

    // Connect the range truths channel notifications.
    //
    connect(configuration->getRangeTruthsChannel(),
            SIGNAL(incoming(const MessageList&)),
            SLOT(processRangeTruths(const MessageList&)));
    connect(configuration->getRangeTruthsChannel(),
            SIGNAL(valueChanged(int)),
            SLOT(rangeTruthsChannelChanged(int)));

    // Connect the user bug plot channel notifications.
    //
    connect(configuration->getBugPlotsChannel(),
            SIGNAL(incoming(const MessageList&)),
            SLOT(processBugPlots(const MessageList&)));
    connect(configuration->getBugPlotsChannel(),
            SIGNAL(valueChanged(int)),
            SLOT(bugPlotsChannelChanged(int)));

    // When gain or min/max cutoff sliders change, redraw the video display.
    //
    connect(configuration->getVideoSampleCountTransform(),
            SIGNAL(settingChanged()), SLOT(redisplayVideo()));

    // When the use pans or zooms, adjust the viewport and redraw.
    //
    viewSettings_ = configuration->getViewSettings();
    connect(viewSettings_, SIGNAL(settingChanged()),
            SLOT(viewSettingsChanged()));

    historySettings_ = configuration->getHistorySettings();

    // Detect changes to the video imaging settings
    //
    videoImaging_ = configuration->getVideoImaging();
    connect(videoImaging_, SIGNAL(settingChanged()),
            SLOT(redisplayVideo()));
    connect(videoImaging_, SIGNAL(colorChanged()),
            SLOT(redisplayVideo()));
    connect(videoImaging_, SIGNAL(alphaChanged()),
            SLOT(redisplayVideo()));
    connect(videoImaging_, SIGNAL(decimationChanged(int)), 
            SLOT(redisplayVideo()));

    // Detect changes to the binary video imaging settings
    //
    binaryImaging_ = configuration->getBinaryImaging();
    connect(binaryImaging_, SIGNAL(sizeChanged()),
            SLOT(redisplayBinary()));
    connect(binaryImaging_, SIGNAL(colorChanged()),
            SLOT(redisplayBinary()));
    connect(binaryImaging_, SIGNAL(alphaChanged()),
            SLOT(redisplayBinary()));
    connect(binaryImaging_, SIGNAL(decimationChanged(int)),
            SLOT(redisplayBinary()));

    // Detect changes to the extraction target imaging settings.
    //
    extractionsImaging_ = configuration->getExtractionsImaging();

    // Detect changes to the range truths target imaging settings
    //
    rangeTruthsImaging_ = configuration->getRangeTruthsImaging();

    // Detect changes to the user bug plot imaging settings.
    //
    bugPlotsImaging_ = configuration->getBugPlotsImaging();

    // Detect changes to the range map imaging settings.
    //
    rangeMapImaging_ = configuration->getRangeMapImaging();
    connect(rangeMapImaging_, SIGNAL(settingChanged()),
            SLOT(remakeRangeMap()));

    // Detect changes to the range rings imaging settings.
    //
    rangeRingsImaging_ = configuration->getRangeRingsImaging();
    connect(rangeRingsImaging_, SIGNAL(settingChanged()),
            SLOT(remakeRangeRings()));

    // Detect changes to the phantom cursor imaging settings.
    //
    phantomCursorImaging_ = configuration->getPhantomCursorImaging();

    // Install the History object that records all incoming data.
    //
    history_ = app->getHistory();

    // Now create offscreen buffers and our display lists.
    //
    makeRangeMapList();

    // Finally, set our Open/GL view transformation.
    //
    setViewTransform();

    // Now that our OpenGL environment is complete, recreate any past magnifier windows that will use it.
    //
    restoreMagnifiers();
}

void
PPIWidget::makeRangeMapList()
{
    glNewList(getDisplayList(kRangeMap), GL_COMPILE);
    {
	rangeMapImaging_->getColor().use();
	glLineWidth(rangeMapImaging_->getSize());
	rangeMap_->render();
    }
    glEndList();
}

void
PPIWidget::makeRangeRingsList()
{
    static Logger::ProcLog log("makeRangeRingsList", Log());

    VertexColorArray points;

    // All vertices have the same color, so ignore the color component in the VertexColorArray.
    //
    glDisableClientState(GL_COLOR_ARRAY);

    glNewList(getDisplayList(kRangeRings), GL_COMPILE);

    rangeRingsImaging_->getColor().use();

    double lineWidth = rangeRingsImaging_->getSize();
    glLineWidth(lineWidth);

    double rangeRingSpacing = rangeRingsImaging_->getRangeSpacing();
    // int rangeTicks = rangeRingsImaging_->getRangeTicks();
    // int azimuthTicks = rangeRingsImaging_->getAzimuthTicks();

    double azimuthSpacing = Utils::degreesToRadians(
	rangeRingsImaging_->getAzimuthSpacing());
    // double azimuthTickSpacing = azimuthSpacing / azimuthTicks;
    // double rangeTickSpacing = rangeRingSpacing / rangeTicks;

    double azimuthMin = viewSettings_->getAzimuthMin();
    double azimuthMax = viewSettings_->getAzimuthMax();
    double rangeMin = viewSettings_->getRangeMin();
    double rangeMax = viewSettings_->getRangeMax();

    // Draw the range lines.
    //
    for (double range = rangeRingSpacing; range < rangeMax;
         range += rangeRingSpacing) {
	points.push_back(Vertex(azimuthMin, range));
	points.push_back(Vertex(azimuthMax, range));
    }

    // Draw the azimuth lines.
    //
    double tickExtent = lineWidth * azimuthScaling_;
    LOGDEBUG << "lineWidth: " << lineWidth << " azimuthScale: "
	     << azimuthScaling_ << " tickExtent: " << tickExtent << std::endl;

    for (double azimuth = azimuthSpacing; azimuth < azimuthMax;
         azimuth += azimuthSpacing) {
	points.push_back(Vertex(azimuth, rangeMin));
	points.push_back(Vertex(azimuth, rangeMax));

    }

    for (double azimuth = 0.0; azimuth >= azimuthMin;
         azimuth -= azimuthSpacing) {
	points.push_back(Vertex(azimuth, rangeMin));
	points.push_back(Vertex(azimuth, rangeMax));
    }

    points.draw(GL_LINES);
    glEndList();

    glEnableClientState(GL_COLOR_ARRAY);
}

void
PPIWidget::processVideo(const MessageList& data)
{
    static Logger::ProcLog log("processVideo", Log());
    LOGINFO << "data->size: " << data.size() << std::endl;

    if (! updateTimer_.isActive() || ! videoBuffer_)
	return;

    size_t index = 0;
    if (trimVideo_) {
	trimVideo_ = false;
	index = data.size() - 1;
	LOGWARNING << "dropping " << (data.size() - 1) << " messages"
		   << std::endl;
    }

    for (; index < data.size(); ++index) {
	Messages::Video::Ref msg(
	    boost::dynamic_pointer_cast<Messages::Video>(data[index]));
	LOGDEBUG << msg->getSequenceCounter() << std::endl;
	if (std::abs(viewSettings_->getRangeFactor() -
                     msg->getRangeFactor()) > 0.001) {
	    viewSettings_->setRangeFactorAndMax(msg->getRangeFactor(),
                                                msg->getRangeMax());
	}

	info_ = msg;

	double azimuth = viewSettings_->normalizedAzimuth(
	    msg->getAzimuthStart());

	if (lastAzimuth_ > azimuth + Utils::kCircleRadians * 0.9) {
	    LOGINFO << "seq: " << msg->getSequenceCounter() << " lastAzimuth: "
		    << lastAzimuth_ << " azimuth: " << azimuth << std::endl;
	    makeCurrent();
	    renderScene(this, kCaptureBuffer);
	    QImage frame = grabFrameBuffer();
	    App::GetApp()->addFrame(frame); 
	    history_->wrap();
	}

	lastAzimuth_ = azimuth;
	history_->addVideo(msg);
	videoVertexGenerator_->add(msg);
    }
}

void
PPIWidget::repaintVideo(const History::MessageVector& video)
{
    Logger::ProcLog log("repaintVideo", Log());
    LOGINFO << "video.size(): " << video.size() << std::endl;
    if (! updateTimer_.isActive() || ! videoBuffer_)
	return;
    if (video.empty()) return;
    videoVertexGenerator_->add(video);
}

void
PPIWidget::processBinary(const MessageList& data)
{
    static Logger::ProcLog log("processBinary", Log());
    LOGDEBUG << data.size() << std::endl;

    if (! updateTimer_.isActive() || ! binaryBuffer_)
	return;

    size_t index = 0;
    if (trimBinary_) {
	trimBinary_ = false;
	index = data.size() - 1;
	LOGWARNING << "dropping " << (data.size() - 1) << " messages"
		   << std::endl;
    }

    for (; index < data.size(); ++index) {
	Messages::BinaryVideo::Ref msg(
	    boost::dynamic_pointer_cast<Messages::BinaryVideo>(
		data[index]));
	double azimuth = viewSettings_->normalizedAzimuth(
	    msg->getAzimuthStart());
	if (! viewSettings_->viewingAzimuth(azimuth))
	    continue;
	history_->addBinary(msg);
	binaryVertexGenerator_->add(msg);
    }
}

void
PPIWidget::repaintBinary(const History::MessageVector& binary)
{
    Logger::ProcLog log("repaintBinary", Log());
    LOGINFO << "binary.size(): " << binary.size() << std::endl;
    if (! updateTimer_.isActive() || ! binaryBuffer_)
	return;
    if (binary.empty()) return;
    binaryVertexGenerator_->add(binary);
}

void
PPIWidget::processExtractions(const MessageList& data)
{
    static Logger::ProcLog log("processExtractions", Log());
    LOGINFO << std::endl;

    if (! updateTimer_.isActive())
	return;

    for (size_t index = 0; index < data.size(); ++index) {
	Messages::Extractions::Ref msg(
	    boost::dynamic_pointer_cast<Messages::Extractions>(
		data[index]));
	history_->addExtractions(msg);
    }
}

void
PPIWidget::processRangeTruths(const MessageList& data)
{
    static Logger::ProcLog log("processRangeTruths", Log());
    LOGINFO << std::endl;

    if (! updateTimer_.isActive())
	return;

    for (size_t index = 0; index < data.size(); ++index) {
	Messages::TSPI::Ref msg(
	    boost::dynamic_pointer_cast<Messages::TSPI>(data[index]));
	LOGERROR << "range: " << msg->getRange()
		 << " azimuth: " << msg->getAzimuth()
		 << " elevation: " << msg->getElevation()
		 << std::endl;
	history_->addRangeTruth(msg);
    }
}

void
PPIWidget::processBugPlots(const MessageList& data)
{
    static Logger::ProcLog log("processBugPlots", Log());
    LOGINFO << std::endl;

    if (! updateTimer_.isActive())
	return;

    for (size_t index = 0; index < data.size(); ++index) {
	Messages::BugPlot::Ref msg(
	    boost::dynamic_pointer_cast<Messages::BugPlot>(data[index]));
	
	history_->addBugPlot(msg);
    }
}

void
PPIWidget::drawExtractions(QWidget* widget)
{
    const TargetPlotList& extractions(history_->getExtractions());
    if (! extractions.empty())
	extractionsImaging_->render(widget, extractions);
}

void
PPIWidget::drawRangeTruths(QWidget* widget)
{
    const TargetPlotListList& rangeTruths(history_->getRangeTruths());
    if (! rangeTruths.empty())
	rangeTruthsImaging_->render(widget, rangeTruths);
}

void
PPIWidget::drawBugPlots(QWidget* widget)
{
    const TargetPlotList& bugPlots(history_->getBugPlots());
    if (! bugPlots.empty())
	bugPlotsImaging_->render(widget, bugPlots);
}

void
PPIWidget::paintGL()
{
    static Logger::ProcLog log("paintGL", Log());

    if (! videoBuffer_)
	makeVideoBuffer();

    if (! binaryBuffer_)
	makeBinaryBuffer();

    renderScene(this, kMainWindow);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewSettings_->getAzimuthMin(), viewSettings_->getAzimuthMax(), viewSettings_->getRangeMin(),
            viewSettings_->getRangeMax(), -1.0, 1.0);

    glEnable(GL_BLEND);

    // Draw asweep indicator at the last azimuth
    //
    double sweep = viewSettings_->normalizedAzimuth(lastAzimuth_);
    double azimuthMin = viewSettings_->getAzimuthMin();
    double azimuthMax = viewSettings_->getAzimuthMax();
    double rangeMin = viewSettings_->getRangeMin();
    double rangeMax = viewSettings_->getRangeMax();

    glColor4f(1.0, 1.0, 1.0, 0.5);
    glLineWidth(1.0);

    if (sweep >= azimuthMin && sweep < azimuthMax) {
	glBegin(GL_LINES);
	glVertex2f(sweep, rangeMin - 10.0);
	glVertex2f(sweep, rangeMax + 10.0);
	glEnd();
    }

    if (rubberBanding_) {
	glBegin(GL_LINE_LOOP);
	glVertex2f(magStartX_, magStartY_);
	glVertex2f(magEndX_, magStartY_);
	glVertex2f(magEndX_, magEndY_);
	glVertex2f(magStartX_, magEndY_);
	glEnd();
    }

    for (int index = 0; index < magnifiers_.size(); ++index) {
	MagnifierWindow* magnifier = magnifiers_[index];
	if (magnifier->showOutline()) {
	    magnifier->drawFrame();
	}
    }

    if (! isActiveWindow() && ! underMouse() && showPhantomCursor_)
	phantomCursorImaging_->drawCursor(phantomCursor_, xScale_, yScale_);

    glDisable(GL_BLEND);
}

void
PPIWidget::renderScene(QGLWidget* widget, RenderType renderType)
{
    static Logger::ProcLog log("renderScene", Log());

    bool capturing = renderType == kCaptureBuffer;

    if (renderType != kMagnifierWindow) {

	if (videoVertexGenerator_->hasData()) {
	    videoVertexGenerator_->processQueue(capturing ? -1 : 100);
	    videoVertexGenerator_->renderInto(videoBuffer_);
	    videoVertexGenerator_->flushPoints();
	}

	if (binaryVertexGenerator_->hasData()) {
	    binaryVertexGenerator_->processQueue(capturing ? -1 : 100);
	    binaryVertexGenerator_->renderInto(binaryBuffer_);
	    binaryVertexGenerator_->flushPoints();
	}
    }

    glDisable(GL_BLEND);

    // NOTE: expect glMatrixMode(GL_PROJECTION) to be in effect at this point.
    //
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Start with texture imaging. The first time we draw a texture, we do so without blending. After that, we
    // blend textures on top of the current image.
    //
    bool showingVideo = videoImaging_->isEnabled();
    if (showingVideo) {
	GLEC(glCallList(getDisplayList(kRenderVideoTexture)));
	glEnable(GL_BLEND);
    }

    // Render on top any binary imaging.
    //
    if (binaryImaging_->isEnabled()) {
	if (showingVideo) {

	    // Enable the following to properly draw the binary texture ontop of the of the normal video data
	    // such that only binary true values appear in the mix. If we are not showing the video data, then
	    // there is no need for the additional work.
	    //
	    glEnable(GL_ALPHA_TEST);
	    glEnable(GL_COLOR_LOGIC_OP);
	}
	GLEC(glCallList(getDisplayList(kRenderBinaryTexture)));
	if (showingVideo) {
	    glDisable(GL_COLOR_LOGIC_OP);
	    glDisable(GL_ALPHA_TEST);
	}
    }

    glEnable(GL_BLEND);

    // The following overlays use the stencil buffer to keep from drawing multiple times over the same spot.
    // Without this protection, pixels drawn with an alpha value < 1.0 will have different alpha values
    // depending on how many times they were drawn on by a feature. Note that for now, the buffer is cleared for
    // each overlay.
    //
    {
	StencilBufferState stencilState;
	if (rangeMapImaging_->isEnabled() &&
            (! capturing || historySettings_->getFrameHasRangeMap())) {
	    stencilState.use();
	    glCallList(getDisplayList(kRangeMap));
	}

	if (rangeRingsImaging_->isEnabled() &&
            (! capturing || historySettings_->getFrameHasGrid())) {
	    stencilState.use();
	    glCallList(getDisplayList(kRangeRings));
	}
    }
    
    if (extractionsImaging_->isEnabled() &&
        (! capturing || historySettings_->getFrameHasExtractions())) {
	drawExtractions(widget);
    }

    if (rangeTruthsImaging_->isEnabled() &&
        (! capturing || historySettings_->getFrameHasRangeTruths())) {
	drawRangeTruths(widget);
    }

    if (bugPlotsImaging_->isEnabled() &&
        (! capturing || historySettings_->getFrameHasBugPlots())) {
	drawBugPlots(widget);
    }

    glDisable(GL_BLEND);
}

void
PPIWidget::remakeRangeMap()
{
    makeCurrent();
    makeRangeMapList();
}

void
PPIWidget::remakeRangeRings()
{
    makeCurrent();
    makeRangeRingsList();
}

void
PPIWidget::resizeGL(int width, int height)
{
    static Logger::ProcLog log("resizeGL", Log());
    glViewport(0, 0, width, height);
    setViewTransform();
    makeVideoBuffer();
    redisplayVideo();
    makeBinaryBuffer();
    redisplayBinary();
}

void
PPIWidget::setViewTransform()
{
    static Logger::ProcLog log("setViewTransform", Log());

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewSettings_->getAzimuthMin(), viewSettings_->getAzimuthMax(),
            viewSettings_->getRangeMin(), viewSettings_->getRangeMax(), -1.0, 1.0);

    xScale_ = viewSettings_->getAzimuthSpan() / width();
    yScale_ = viewSettings_->getRangeSpan() / height();
    azimuthScaling_ = xScale_ / yScale_;
    makeRangeRingsList();
    makeRangeMapList();

    emit transformChanged();
}

void
PPIWidget::viewSettingsChanged()
{
    makeCurrent();
    setViewTransform();
}

void
PPIWidget::redisplayAll()
{
    makeCurrent();
    repaintVideo(history_->getVideo());
    if (binaryImaging_->isEnabled())
	repaintBinary(history_->getBinary());
}

void
PPIWidget::clearAll()
{
    makeCurrent();
    clearVideoBuffer();
    clearBinaryBuffer();
    history_->clear();
}

void
PPIWidget::clearVideoBuffer()
{
    makeCurrent();
    videoVertexGenerator_->flushAll();
    if (videoBuffer_)
	videoBuffer_->clearBuffer();
}

void
PPIWidget::clearBinaryBuffer()
{
    makeCurrent();
    binaryVertexGenerator_->flushAll();
    if (binaryBuffer_)
	binaryBuffer_->clearBuffer();
}

void
PPIWidget::clearExtractions()
{
    history_->clearExtractions();
}

void
PPIWidget::clearRangeTruths()
{
    history_->clearRangeTruths();
}

void
PPIWidget::clearBugPlots()
{
    history_->clearBugPlots();
}

void
PPIWidget::localToRealWorld(int x, int y, GLdouble& azimuth, GLdouble& range)
    const
{
    static Logger::ProcLog log("localToRealWorld", Log());
    azimuth = x * xScale_ + viewSettings_->getAzimuthMin();
    range = viewSettings_->getRangeMax() - y * yScale_;
}

void
PPIWidget::updateCursorPosition(CursorPosition& pos)
{
    if (history_) {
	bool isValid = false;

	if (videoImaging_->isEnabled()) {
	    Messages::Video::DatumType datum = history_->getVideoValue(
		pos.getAzimuth(), pos.getRange(), isValid);
	    if (isValid)
		pos.setSampleValue(QString::number(datum));
	}
	else if (binaryImaging_->isEnabled()) {
	    Messages::BinaryVideo::DatumType datum = history_->getBinaryValue(
		pos.getAzimuth(), pos.getRange(), isValid);
	    if (isValid)
		pos.setSampleValue(datum ? "T" : "F");
	}
    }
}

void
PPIWidget::showCursorInfo()
{
    GLdouble azimuth, range;
    localToRealWorld(mouse_.x(), mouse_.y(), azimuth, range);
    CursorPosition pos(viewSettings_->azimuthToParametric(azimuth),
                       viewSettings_->rangeToParametric(range),
                       azimuth, range);
    updateCursorPosition(pos);
    setCursorPosition(pos.getToolTip());

    emit currentCursorPosition(pos.getXY());
}

void
PPIWidget::timerEvent(QTimerEvent* event)
{
    static Logger::ProcLog log("timerEvent", Log());

    if (event->timerId() != updateTimer_.timerId()) {
	event->ignore();
	Super::timerEvent(event);
	return;
    }

    updateGL();

    if (underMouse()) {
	QPoint newMouse = mapFromGlobal(QCursor::pos());
	if (newMouse != mouse_) {
	    mouse_ = newMouse;
	    showCursorInfo();
	}
    }

    if (info_) {
	emit currentMessage(info_);
	info_.reset();
    }
}

void
PPIWidget::showEvent(QShowEvent* event)
{
    Logger::ProcLog log("showEvent", Log());
    LOGINFO << std::endl;
    if (! updateTimer_.isActive())
	updateTimer_.start(kUpdateRate, this);
    Super::showEvent(event);
}

void
PPIWidget::closeEvent(QCloseEvent* event)
{
    Logger::ProcLog log("closeEvent", Log());
    LOGINFO << std::endl;
    if (updateTimer_.isActive())
	updateTimer_.stop();
    Super::closeEvent(event);
}

void
PPIWidget::videoChannelChanged(int index)
{
    if (index) {
	history_->clearVideo();
	clearVideoBuffer();
    }
}

void
PPIWidget::binaryChannelChanged(int index)
{
    if (index) {
	history_->clearBinary();
	clearBinaryBuffer();
    }
}

void
PPIWidget::extractionsChannelChanged(int index)
{
    if (index)
	history_->clearExtractions();
}

void
PPIWidget::rangeTruthsChannelChanged(int index)
{
    if (index)
	history_->clearRangeTruths();
}

void
PPIWidget::bugPlotsChannelChanged(int index)
{
    if (index)
	history_->clearBugPlots();
}

void
PPIWidget::setPhantomCursor(const QPointF& pos)
{
    phantomCursor_ = QPointF(
	viewSettings_->azimuthFromParametric(pos.x()),
	viewSettings_->rangeFromParametric(pos.y()));
}

void
PPIWidget::redisplayVideo()
{
    repaintVideo(history_->getVideo());
}

void
PPIWidget::redisplayBinary()
{
    repaintBinary(history_->getBinary());
}

void
PPIWidget::leaveEvent(QEvent* event)
{
    phantomCursor_ = PhantomCursorImaging::InvalidCursor();
    Super::leaveEvent(event);
    if (rubberBanding_) {
	rubberBanding_ = false;
    }
}

void
PPIWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
	if (event->modifiers() == 0) {
	    event->accept();
	    magStartPos_ = event->pos();
	    localToRealWorld(event->x(), event->y(), magStartX_, magStartY_);
	    magEndX_ = magStartX_;
	    magEndY_ = magStartY_;
	    rubberBanding_ = true;
	}
	else if (event->modifiers() == Qt::ControlModifier) {
	    event->accept();
	    GLdouble range, azimuth;
	    localToRealWorld(event->x(), event->y(), azimuth, range);
	    azimuth = Utils::normalizeRadians(azimuth);
	    Messages::BugPlot::Ref msg =
		bugPlotEmitterSettings_->addBugPlot(range, azimuth);
	    if (msg)
		history_->addBugPlot(msg);
	}
    }
    Super::mousePressEvent(event);
}

void
PPIWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (rubberBanding_) {
	localToRealWorld(event->x(), event->y(), magEndX_, magEndY_);
    }
}

void
PPIWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Logger::ProcLog log("mouseReleaseEvent", Log());
    LOGINFO << std::endl;

    if (event->button() == Qt::LeftButton) {
	if (rubberBanding_) {
	    event->accept();
	    rubberBanding_ = false;
	    if (std::abs(magStartPos_.x() - event->x()) > 20 &&
                std::abs(magStartPos_.y() - event->y()) > 20) {
		MagnifierWindow* magnifier = new MagnifierWindow(this);
		magnifier->initialize();
		localToRealWorld(event->x(), event->y(), magEndX_, magEndY_);
		magnifier->setBounds(std::min(magStartX_, magEndX_),
                                     std::min(magStartY_, magEndY_),
                                     std::abs(magStartX_ - magEndX_),
                                     std::abs(magStartY_ - magEndY_),
                                     xScale_, yScale_);
		magnifier->showAndRaise();
	    }
	}
    }

    Super::mouseReleaseEvent(event);
}

void
PPIWidget::addMagnifier(MagnifierWindow* magnifier)
{
    magnifiers_.append(magnifier);
}

void
PPIWidget::removeMagnifier(MagnifierWindow* magnifier)
{
    if (! magnifier)
	magnifier = static_cast<MagnifierWindow*>(sender());
    magnifiers_.removeAll(magnifier);
    phantomCursor_ = phantomCursorImaging_->InvalidCursor();
    update();
}

void
PPIWidget::saveMagnifiers(QSettings& settings) const
{
    Logger::ProcLog log("saveMagnifiers", Log());
    LOGERROR << "count: " << magnifiers_.size() << " group: "
	     << settings.group() << std::endl;
    settings.beginWriteArray("Magnifiers", magnifiers_.size());
    for (int index = 0; index < magnifiers_.size(); ++index) {
	settings.setArrayIndex(index);
	magnifiers_[index]->save(settings);
    }
    settings.endArray();
}

void
PPIWidget::restoreMagnifiers()
{
    Logger::ProcLog log("restoreMagnifiers", Log());
    LOGERROR << "settingsKey: " << settingsKey_ << std::endl;

    QSettings settings;
    settings.beginGroup(settingsKey_);

    // Restore any previous magnification views. NOTE: creating a new MagnifierWindow will install a different
    // Open/GL context. Use makeCurrent() to restore ours.
    //
    int count = settings.beginReadArray("Magnifiers");
    LOGERROR << "count: " << count << std::endl;
    for (int index = 0; index < count; ++index) {
	settings.setArrayIndex(index);
	MagnifierWindow* magnifier = new MagnifierWindow(this);
	magnifier->initialize();
	magnifier->restore(settings);
	makeCurrent();
    }
    settings.endArray();
}

void
PPIWidget::showPhantomCursor(bool state)
{
    showPhantomCursor_ = state;
}

void
PPIWidget::raiseMagnifiers()
{
    Logger::ProcLog log("raiseMagnifiers", Log());
    LOGINFO << std::endl;
    foreach(MagnifierWindow* mag, magnifiers_)
	mag->showAndRaise();
}

void
PPIWidget::setShowCursorPosition(bool state)
{
    showCursorPosition_ = state;
    setToolTip("");
    if (state)
	setToolTip(cursorPosition_);
}

void
PPIWidget::setCursorPosition(const QString& value)
{
    if (value != cursorPosition_) {
	cursorPosition_ = value;
	if (showCursorPosition_)
	    setToolTip(cursorPosition_);
    }
    else {
	setToolTip("");
	setToolTip(value);
    }
}
