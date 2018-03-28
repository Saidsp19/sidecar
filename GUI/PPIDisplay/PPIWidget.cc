#include <math.h>

#include "QtCore/QSettings"
#include "QtGui/QCursor"
#include "QtGui/QImage"
#include "QtGui/QMessageBox"
#include "QtGui/QStatusBar"

#include "GUI/BugPlotEmitterSettings.h"
#include "GUI/ChannelSetting.h"
#include "GUI/GLInitLock.h"
#include "GUI/LogUtils.h"
#include "GUI/MessageList.h"
#include "GUI/PhantomCursorImaging.h"
#include "GUI/RangeMap.h"
#include "GUI/RangeRingsImaging.h"
#include "GUI/RangeTruthsImaging.h"
#include "GUI/StencilBufferState.h"
#include "GUI/TargetPlotImaging.h"
#include "GUI/VideoSampleCountTransform.h"
#include "IO/MessageManager.h"
#include "Utils/SineCosineLUT.h"
#include "Utils/Utils.h"

#include "App.h"
#include "BackgroundImageSettings.h"
#include "BinaryVertexGenerator.h"
#include "Configuration.h"
#include "CursorPosition.h"
#include "DecaySettings.h"
#include "MagnifierWindow.h"
#include "MainWindow.h"
#include "OffscreenBuffer.h"
#include "PPIWidget.h"
#include "VideoImaging.h"
#include "VideoVertexGenerator.h"
#include "ViewSettings.h"

using namespace Utils;
using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::PPIDisplay;

static int kUpdateRate = 33; // msecs between update() calls (~30 FPS)

Logger::Log&
PPIWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ppidisplay.PPIWidget");
    return log_;
}

QGLFormat
PPIWidget::GetGLFormat()
{
    QGLFormat format = QGLFormat::defaultFormat();
    format.setAlpha(true);
    format.setDepth(false);
    format.setSampleBuffers(true);
    return format;
}

PPIWidget::PPIWidget(QWidget* parent) :
    Super(GetGLFormat(), parent), xSpan_(1.0), ySpan_(1.0), lastAzimuth_(0.0), lastShaftEncoding_(0), viewSettings_(0),
    videoImaging_(0), binaryImaging_(0), extractionsImaging_(0), rangeTruthsImaging_(0), bugPlotsImaging_(0),
    rangeMapImaging_(0), rangeRingsImaging_(0), backgroundImageSettings_(0), decaySettings_(0),
    phantomCursorImaging_(0), bugPlotEmitterSettings_(0), rangeMap_(new RangeMap), history_(0), videoBuffer_(0),
    videoVertexGenerator_(new VideoVertexGenerator), binaryBuffer_(0),
    binaryVertexGenerator_(new BinaryVertexGenerator), displayLists_(0), decayTexture_(), desaturationTexture_(),
    backgroundTexture_(), updateTimer_(), phantomCursor_(PhantomCursorImaging::InvalidCursor()), info_(),
    settingsKey_(), magnifiers_(), cursorPosition_(), panning_(false), rubberBanding_(false), showPhantomCursor_(true),
    showCursorPosition_(true), trimVideo_(true), trimBinary_(true)
{
    Logger::ProcLog log("PPIWidget", Log());
    LOGINFO << std::endl;

    // Widget initialization. NOTE: all Open/GL initialization occurs in the initializeGL method.
    //
    setCursor(Qt::CrossCursor);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
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
    glDeleteLists(displayLists_, kNumLists);
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
    int span = std::min(width(), height());
    videoBuffer_ = new OffscreenBuffer(videoImaging_, viewSettings_->getRangeMax(), span, span);
    if (!videoBuffer_->isValid()) {
        QMessageBox::critical(window(), "Bad Format",
                              "Failed to create offscreen texture to display "
                              "Video message data.");
    }
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

    makeCurrent();
    int span = std::min(width(), height());
    binaryBuffer_ = new OffscreenBuffer(binaryImaging_, viewSettings_->getRangeMax(), span, span);
    if (!binaryBuffer_->isValid()) {
        QMessageBox::critical(window(), "Bad Format",
                              "Failed to create offscreen texture to display "
                              "BinaryVideo message data.");
    }
    makeCurrent();
    makeRenderTextureList(kRenderBinaryTexture, binaryBuffer_->getTexture());
}

void
PPIWidget::makeRenderTextureList(ListIndex index, const Texture& texture)
{
    Logger::ProcLog log("makeRenderTextureList", Log());
    LOGINFO << "index: " << index << " texture: " << texture.getId() << std::endl;

    float span = viewSettings_->getRangeMax();
    GLEC(glNewList(getDisplayList(index), GL_COMPILE));
    {
        GLEC(glColor4f(1.0, 1.0, 1.0, 1.0));
        GLEC(glEnable(texture.getType()));
        GLEC(glBindTexture(texture.getType(), texture.getId()));
        GLEC(glBegin(GL_QUADS));
        GLEC(glTexCoord2f(texture.getXMin(), texture.getYMin()));
        GLEC(glVertex2f(-span, -span));
        GLEC(glTexCoord2f(texture.getXMax(), texture.getYMin()));
        GLEC(glVertex2f(span, -span));
        GLEC(glTexCoord2f(texture.getXMax(), texture.getYMax()));
        GLEC(glVertex2f(span, span));
        GLEC(glTexCoord2f(texture.getXMin(), texture.getYMax()));
        GLEC(glVertex2f(-span, span));
        GLEC(glEnd());
        GLEC(glBindTexture(texture.getType(), 0));
        GLEC(glDisable(texture.getType()));
    }
    GLEC(glEndList());
}

void
PPIWidget::initializeContext(GLdouble* modelViewMatrix)
{
    // Enable the use of vertex and color arrays when calling glDrawArrays(). Additional setup happens inside
    // the VertexColorArray::draw() method.
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
    // causes unequal alpha values in their rendering.
    //
    glClearStencil(1);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

    // Set model view to identity matrix (it never changes). Fetch the components of the matrix so we can use it
    // in UnProjectPoint to convert mouse coordinates to real-world units.
    //
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (modelViewMatrix) { glGetDoublev(GL_MODELVIEW_MATRIX, modelViewMatrix); }

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

    initializeContext(modelViewMatrix_);

    // Receive phantom cursor updates from the singleton App instance.
    //
    App* app = App::GetApp();
    connect(this, SIGNAL(currentCursorPosition(const QPointF&)), app, SLOT(setPhantomCursor(const QPointF&)));
    connect(app, SIGNAL(phantomCursorChanged(const QPointF&)), SLOT(setPhantomCursor(const QPointF&)));

    // Now that our Open/GL state is adequately setup, install the various confiugration and imaging objects.
    //
    Configuration* configuration = app->getConfiguration();
    bugPlotEmitterSettings_ = configuration->getBugPlotEmitterSettings();

    // Acquire the application's sine/cosine lookup table.
    //
    sineCosineLUT_ = configuration->getSineCosineLUT();

    // Connect the video channel notifications.
    //
    connect(configuration->getVideoChannel(), SIGNAL(incoming(const MessageList&)),
            SLOT(processVideo(const MessageList&)));
    connect(configuration->getVideoChannel(), SIGNAL(valueChanged(const QString&)),
            SLOT(videoChannelChanged(const QString&)));

    // Connect the binary video channel notifications.
    //
    connect(configuration->getBinaryChannel(), SIGNAL(incoming(const MessageList&)),
            SLOT(processBinary(const MessageList&)));
    connect(configuration->getBinaryChannel(), SIGNAL(valueChanged(const QString&)),
            SLOT(binaryChannelChanged(const QString&)));

    // Connect the extractions channel notifications.
    //
    connect(configuration->getExtractionsChannel(), SIGNAL(incoming(const MessageList&)),
            SLOT(processExtractions(const MessageList&)));
    connect(configuration->getExtractionsChannel(), SIGNAL(valueChanged(const QString&)),
            SLOT(extractionsChannelChanged(const QString&)));

    // Connect the range truths channel notifications.
    //
    connect(configuration->getRangeTruthsChannel(), SIGNAL(incoming(const MessageList&)),
            SLOT(processRangeTruths(const MessageList&)));
    connect(configuration->getRangeTruthsChannel(), SIGNAL(valueChanged(const QString&)),
            SLOT(rangeTruthsChannelChanged(const QString&)));

    // Connect the bug plots channel notifications.
    //
    connect(configuration->getBugPlotsChannel(), SIGNAL(incoming(const MessageList&)),
            SLOT(processBugPlots(const MessageList&)));
    connect(configuration->getBugPlotsChannel(), SIGNAL(valueChanged(const QString&)),
            SLOT(bugPlotsChannelChanged(const QString&)));

    // When gain or min/max cutoff sliders change, redraw the video display.
    //
    connect(configuration->getVideoSampleCountTransform(), SIGNAL(settingChanged()), SLOT(redisplayVideo()));

    // When the user pans or zooms, adjust the viewport and redraw.
    //
    viewSettings_ = configuration->getViewSettings();
    connect(viewSettings_, SIGNAL(settingChanged()), SLOT(viewSettingsChanged()));
    connect(viewSettings_, SIGNAL(rangeMaxChanged(double)), SLOT(rangeMaxChanged(double)));
    connect(viewSettings_, SIGNAL(rangeMaxMaxChanged(double)), SLOT(rangeMaxMaxChanged(double)));

    // Detect changes to the video imaging settings.
    //
    videoImaging_ = configuration->getVideoImaging();
    connect(videoImaging_, SIGNAL(colorChanged()), SLOT(redisplayVideo()));
    connect(videoImaging_, SIGNAL(sizeChanged()), SLOT(redisplayVideo()));
    connect(videoImaging_, SIGNAL(colorMapChanged(const QImage&)), SLOT(remakeDesaturationTexture()));
    connect(videoImaging_, SIGNAL(desaturationChanged()), SLOT(remakeDesaturationTexture()));

    // Detect changes to the binary video imaging settings.
    //
    binaryImaging_ = configuration->getBinaryImaging();
    connect(binaryImaging_, SIGNAL(colorChanged()), SLOT(redisplayBinary()));
    connect(binaryImaging_, SIGNAL(sizeChanged()), SLOT(redisplayBinary()));

    // Detect changes to the extraction target imaging settings
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
    connect(rangeMapImaging_, SIGNAL(settingChanged()), SLOT(remakeRangeMap()));

    // Detect changes to the range rings imaging settings.
    //
    rangeRingsImaging_ = configuration->getRangeRingsImaging();
    connect(rangeRingsImaging_, SIGNAL(settingChanged()), SLOT(remakeRangeRings()));

    // Detect changes to the background image imaging settings.
    //
    backgroundImageSettings_ = configuration->getBackgroundImageSettings();
    connect(backgroundImageSettings_, SIGNAL(imageChanged(const QImage&)), SLOT(makeBackgroundTexture(const QImage&)));

    // Detect changes to the decay mask settings.
    //
    decaySettings_ = configuration->getDecaySettings();
    connect(decaySettings_, SIGNAL(settingChanged()), SLOT(remakeDecayTexture()));

    phantomCursorImaging_ = configuration->getPhantomCursorImaging();

    // Install the History object that records all incoming data. Receive notification when the history view
    // changes or gets older.
    //
    history_ = app->getHistory();
    connect(history_, SIGNAL(currentViewChanged(int)), SLOT(historyCurrentViewChanged(int)));
    connect(history_, SIGNAL(currentViewAged(int)), SLOT(historyCurrentViewAged(int)));

    // Now create offscreen buffers and our display lists.
    //
    makeDecayTexture();
    makeDesaturationTexture();
    makeDecayList();
    makeDesaturationList();
    makeBackgroundTexture(backgroundImageSettings_->getFiltered());

    // Now that our OpenGL environment is complete, recreate any past magnifier windows that will use it.
    //
    restoreMagnifiers();

    resizeGL(width(), height());
}

void
PPIWidget::restoreMagnifiers()
{
    Logger::ProcLog log("restoreMagnifiers", Log());
    LOGINFO << "settingsKey: " << settingsKey_ << std::endl;

    QSettings settings;
    settings.beginGroup(settingsKey_);

    // Restore any previous magnification views. NOTE: creating a new MagnifierWindow may make a different
    // Open/GL context current.
    //
    int count = settings.beginReadArray("Magnifiers");
    LOGDEBUG << "count: " << count << std::endl;
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
PPIWidget::makeDecayTexture()
{
    static Logger::ProcLog log("makeDecayTexture", Log());
    LOGINFO << std::endl;

    // Make the "normal" decay mask. This has increasing alpha values as we go from left to right in the 1D
    // image. The 'distance' parameter controls where the mask goes completely black: a value < 360 will decay
    // faster, while values > 360 will never go completely black.
    //
    QImage band(256, 1, QImage::Format_ARGB32);
    double step = decaySettings_->getDistance() / 360.0;
    LOGDEBUG << "decayDistance: " << decaySettings_->getDistance() << " step: " << step << std::endl;

    for (int index = 0; index < 256; ++index) {
        int alpha = int(::rint(index * step));
        if (alpha > 255)
            alpha = 255;
        else if (alpha < 0)
            alpha = 0;
        LOGDEBUG << index << ' ' << alpha << std::endl;
        band.setPixel(index, 0, QColor(0, 0, 0, alpha).rgba());
    }

    if (decayTexture_) {
        deleteTexture(decayTexture_.getId());
        decayTexture_.invalidate();
    }

    // Create a 2D texture from the 1D image above.
    //
    decayTexture_ = Texture(GL_TEXTURE_2D, bindTexture(band.scaled(256, 256)), 256, 256);
}

void
PPIWidget::buildDiscTexture() const
{
    GLdouble baseRadius = viewSettings_->getRangeMaxMax() * 1.20;
    GLdouble height = 1.0;
    GLint slices = 180;

    glBegin(GL_QUAD_STRIP);
    for (GLint index = 0; index < slices; ++index) {
        GLdouble angle = 2 * M_PI * index / slices;
        GLdouble sinv = ::sin(angle);
        GLdouble cosv = ::cos(angle);
        GLfloat tx = 1.0 - GLfloat(index) / slices;
        glTexCoord2f(tx, 0.0);
        glVertex3f(baseRadius * sinv, baseRadius * cosv, 0.0);
        glTexCoord2f(tx, 1.0);
        glVertex3f(0.0, 0.0, height);
    }
    glEnd();
}

void
PPIWidget::makeDecayList()
{
    Logger::ProcLog log("makeDecayList", Log());

    // Map the decay texture created in makeDecayMask() onto a flattened cylinder of height rangeMax + 1. This
    // will create disk filled with a gradiant that gets darker as one moves CCW from north.
    //
    glNewList(getDisplayList(kDecay), GL_COMPILE);
    {
        glEnable(decayTexture_.getType());
        glBindTexture(decayTexture_.getType(), decayTexture_.getId());
        buildDiscTexture();
        glBindTexture(decayTexture_.getType(), 0);
        glDisable(decayTexture_.getType());
    }

    glEndList();
}

void
PPIWidget::makeDesaturationTexture()
{
    static Logger::ProcLog log("makeDesaturationTexture", Log());

    // The desaturation mask gradually removes certain color components from the image, causing the primary
    // color to remain and thus desaturate the result. The rate control determines how fast the desaturation
    // occurs.
    //
    QImage band(256, 1, QImage::Format_ARGB32);
    double range = 256.0 * videoImaging_->getDesaturateRate();
    double step = 256.0 / range;
    LOGDEBUG << "desaturationRate: " << videoImaging_->getDesaturateRate() << " step: " << step << std::endl;

    for (int index = 0; index < 256; ++index) {
        int alpha;
        if (index > range) {
            alpha = 255;
        } else {
            alpha = int(::rint(index * step));
            if (alpha > 255) {
                alpha = 255;
            } else if (alpha < 0) {
                alpha = 0;
            }
        }

        LOGDEBUG << index << ' ' << alpha << std::endl;
        band.setPixel(index, 0, QColor(0, 0, 0, alpha).rgba());
    }

    if (desaturationTexture_) {
        deleteTexture(desaturationTexture_.getId());
        desaturationTexture_.invalidate();
    }

    // Create a 2D texture from the 1D image above.
    //
    desaturationTexture_ = Texture(GL_TEXTURE_2D, bindTexture(band.scaled(256, 256)), 256, 256);
}

void
PPIWidget::makeDesaturationList()
{
    Logger::ProcLog log("makeDesaturationList", Log());

    glNewList(getDisplayList(kDesaturation), GL_COMPILE);
    {
        // To properly desaturate the image, we must set the correct color mask so that we leave the primary
        // color alone.
        //
        switch (videoImaging_->getCLUT().getType()) {
        case CLUT::kRedSaturated: glColorMask(false, true, true, true); break;

        case CLUT::kGreenSaturated: glColorMask(true, false, true, true); break;

        case CLUT::kBlueSaturated: glColorMask(true, true, false, true); break;

        default: break;
        }

        glEnable(desaturationTexture_.getType());
        glBindTexture(desaturationTexture_.getType(), desaturationTexture_.getId());
        buildDiscTexture();
        glBindTexture(desaturationTexture_.getType(), 0);
        glDisable(desaturationTexture_.getType());

        // After the desaturation has been performed, restore the color masking.
        //
        glColorMask(true, true, true, true);
    }

    glEndList();
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
    VertexColorArray points;

    // All vertices have the same color, so disable color arrays in future glDrawArrays() calls. Remember to
    // reenable at the end.
    //
    glDisableClientState(GL_COLOR_ARRAY);

    glNewList(getDisplayList(kRangeRings), GL_COMPILE);

    rangeRingsImaging_->getColor().use();

    double lineWidth = rangeRingsImaging_->getSize();
    glLineWidth(lineWidth);

    double rangeRingSpacing = rangeRingsImaging_->getRangeSpacing();
    double range = rangeRingSpacing;

    // Draw the range rings.
    //
    while (range <= viewSettings_->getRangeMax()) {
        // For each range ring, fill points with 256 vertices that define a circle.
        //
        size_t increment = sineCosineLUT_->size() / 256;
        for (size_t index = 0; index < sineCosineLUT_->size(); index += increment) {
            double sine;
            double cosine;
            sineCosineLUT_->lookup(index, sine, cosine);
            points.push_back(Vertex(range * sine, range * cosine));
        }

        // Render the ring.
        //
        points.draw(GL_LINE_LOOP);
        points.clear();

        range += rangeRingSpacing;
    }

    // Now draw the radials and tick marks.
    //
    int rangeTicks = rangeRingsImaging_->getRangeTicks();
    int azimuthTicks = rangeRingsImaging_->getAzimuthTicks();

    double azimuthSpacing = rangeRingsImaging_->getAzimuthSpacing();
    double azimuthTickSpacing = azimuthSpacing / azimuthTicks;
    double rangeTickSpacing = rangeRingSpacing / rangeTicks;

    glPushMatrix();

    double azimuth = 0.0;
    int azimuthTickCount = 0;

    // Rotate around a circle, drawing the radial lines and range tick marks.
    //
    while (azimuth < 360.0) {
        // See if we should draw a radial line.
        //
        if (azimuthTickCount++ % azimuthTicks == 0) {
            // Add the vertices for the radial line.
            //
            points.push_back(Vertex(0, rangeTickSpacing));
            points.push_back(Vertex(0, viewSettings_->getRangeMax()));

            // Now draw the tick marks on the radial.
            //
            int rangeTickCount = 0;
            range = rangeTickSpacing;
            while (range < viewSettings_->getRangeMax()) {
                if (++rangeTickCount % rangeTicks != 0) {
                    points.push_back(Vertex(-lineWidth, range));
                    points.push_back(Vertex(lineWidth, range));
                }
                range += rangeTickSpacing;
            }
        } else {
            // Draw inter-radial tick marks.
            //
            range = rangeRingSpacing;
            while (range < viewSettings_->getRangeMax()) {
                points.push_back(Vertex(0.0, range - lineWidth));
                points.push_back(Vertex(0.0, range + lineWidth));
                range += rangeRingSpacing;
            }
        }

        // Draw the radial and ticks at this azimuth.
        //
        points.draw(GL_LINES);
        points.clear();

        // Rotate to the next azimuth value.
        //
        azimuth += azimuthTickSpacing;
        glRotatef(azimuthTickSpacing, 0.0f, 0.0f, -1.0f);
    }

    glPopMatrix();
    glEndList();

    glEnableClientState(GL_COLOR_ARRAY);
}

void
PPIWidget::rangeMaxChanged(double value)
{
    Logger::ProcLog log("rangeMaxChanged", Log());
    LOGINFO << "value: " << value << std::endl;

    if (!displayLists_) return;

    // Remake the display lists that use the rangeMax value.
    //
    makeCurrent();
    if (videoBuffer_) {
        videoBuffer_->clearBuffer();
        makeRenderTextureList(kRenderVideoTexture, videoBuffer_->getTexture());
    }

    if (binaryBuffer_) {
        binaryBuffer_->clearBuffer();
        makeRenderTextureList(kRenderBinaryTexture, binaryBuffer_->getTexture());
    }

    setViewTransform();
}

void
PPIWidget::rangeMaxMaxChanged(double value)
{
    if (!displayLists_) return;
    makeCurrent();
    makeDesaturationList();
    makeDecayList();
}

void
PPIWidget::processVideo(const MessageList& data)
{
    static Logger::ProcLog log("processVideo", Log());
    LOGINFO << "data->size: " << data.size() << std::endl;

    if (!updateTimer_.isActive() || !videoBuffer_) return;

    size_t index = 0;
    if (trimVideo_) {
        trimVideo_ = false;
        index = data.size() - 1;
        LOGWARNING << "dropping " << (data.size() - 1) << " messages" << std::endl;
    }

    Messages::Video::Ref msg;
    for (; index < data.size(); ++index) {
        msg = boost::dynamic_pointer_cast<Messages::Video>(data[index]);
        if (std::abs(viewSettings_->getRangeFactor() - msg->getRangeFactor()) > 0.001) {
            viewSettings_->setRangeFactorAndMax(msg->getRangeFactor(), msg->getRangeMax());
        }

        history_->addVideo(msg);
        if (history_->showingLiveEntry()) { videoVertexGenerator_->add(msg); }
    }

    if (history_->showingLiveEntry()) {
        info_ = msg;
        lastAzimuth_ = radiansToDegrees(msg->getAzimuthStart());
        lastShaftEncoding_ = msg->getShaftEncoding();
    }
}

void
PPIWidget::repaintVideo(const History::MessageVector& video)
{
    Logger::ProcLog log("repaintVideo", Log());
    LOGINFO << "video.size(): " << video.size() << std::endl;
    if (!updateTimer_.isActive() || !videoBuffer_ || video.empty()) { return; }

    videoVertexGenerator_->add(video);
}

void
PPIWidget::processBinary(const MessageList& data)
{
    static Logger::ProcLog log("processBinary", Log());
    LOGDEBUG << data.size() << std::endl;

    if (!updateTimer_.isActive() || !binaryBuffer_) return;

    size_t index = 0;
    if (trimBinary_) {
        trimBinary_ = false;
        index = data.size() - 1;
        LOGWARNING << "dropping " << (data.size() - 1) << " messages" << std::endl;
    }

    for (; index < data.size(); ++index) {
        Messages::BinaryVideo::Ref msg(boost::dynamic_pointer_cast<Messages::BinaryVideo>(data[index]));
        history_->addBinary(msg);
        if (history_->showingLiveEntry()) { binaryVertexGenerator_->add(msg); }
    }
}

void
PPIWidget::repaintBinary(const History::MessageVector& binary)
{
    Logger::ProcLog log("repaintBinary", Log());
    LOGINFO << "binary.size(): " << binary.size() << std::endl;
    if (!updateTimer_.isActive() || !binaryBuffer_) return;
    if (binary.empty()) return;
    binaryVertexGenerator_->add(binary);
}

void
PPIWidget::processExtractions(const MessageList& data)
{
    static Logger::ProcLog log("processExtractions", Log());
    LOGINFO << std::endl;

    if (!updateTimer_.isActive()) return;

    for (size_t index = 0; index < data.size(); ++index) {
        Messages::Extractions::Ref msg(boost::dynamic_pointer_cast<Messages::Extractions>(data[index]));
        history_->addExtractions(msg);
    }
}

void
PPIWidget::processRangeTruths(const MessageList& data)
{
    static Logger::ProcLog log("processRangeTruths", Log());
    LOGINFO << std::endl;

    if (!updateTimer_.isActive()) return;

    for (size_t index = 0; index < data.size(); ++index) {
        Messages::TSPI::Ref msg(boost::dynamic_pointer_cast<Messages::TSPI>(data[index]));
        LOGDEBUG << "range: " << msg->getRange() << " azimuth: " << msg->getAzimuth()
                 << " elevation: " << msg->getElevation() << std::endl;
        history_->addRangeTruth(msg);
    }
}

void
PPIWidget::processBugPlots(const MessageList& data)
{
    static Logger::ProcLog log("processBugPlots", Log());
    LOGINFO << std::endl;

    if (!updateTimer_.isActive()) return;

    for (size_t index = 0; index < data.size(); ++index) {
        Messages::BugPlot::Ref msg(boost::dynamic_pointer_cast<Messages::BugPlot>(data[index]));
        LOGDEBUG << *msg << std::endl;
        history_->addBugPlot(msg);
    }
}

void
PPIWidget::drawExtractions(QWidget* widget)
{
    const TargetPlotList& extractions(history_->getViewedEntry().getExtractions());
    if (!extractions.empty()) extractionsImaging_->render(widget, extractions);
}

void
PPIWidget::drawRangeTruths(QWidget* widget)
{
    const TargetPlotListList& rangeTruths(history_->getViewedEntry().getRangeTruths());
    if (!rangeTruths.empty()) rangeTruthsImaging_->render(widget, rangeTruths);
}

void
PPIWidget::drawBugPlots(QWidget* widget)
{
    const TargetPlotList& bugPlots(history_->getViewedEntry().getBugPlots());
    if (!bugPlots.empty()) bugPlotsImaging_->render(widget, bugPlots);
}

void
PPIWidget::paintGL()
{
    static Logger::ProcLog log("paintGL", Log());

    if (!videoBuffer_) makeVideoBuffer();

    if (!binaryBuffer_) makeBinaryBuffer();

    videoVertexGenerator_->processQueue();
    videoVertexGenerator_->renderInto(videoBuffer_);
    videoVertexGenerator_->flushPoints();

    binaryVertexGenerator_->processQueue();
    binaryVertexGenerator_->renderInto(binaryBuffer_);
    binaryVertexGenerator_->flushPoints();

    renderScene(this);

    glColor4f(1.0, 1.0, 1.0, 0.5);
    glLineWidth(1.0);

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
        if (magnifier->showOutline()) { magnifier->drawFrame(); }
    }

    if (!isActiveWindow() && !underMouse() && showPhantomCursor_)
        phantomCursorImaging_->drawCursor(phantomCursor_, xScale_, yScale_);
}

void
PPIWidget::renderScene(QGLWidget* widget)
{
    static Logger::ProcLog log("renderScene", Log());

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

        // Apply the saturation decay (desaturate) mask if enabled.
        //
        if (videoImaging_->getDesaturateEnabled() && history_->showingLiveEntry()) {
            glPushMatrix();
            glRotatef(lastAzimuth_, 0.0f, 0.0f, -1.0f);
            GLEC(glCallList(getDisplayList(kDesaturation)));
            glPopMatrix();
        }
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

    if (history_->showingLiveEntry()) {
        if (decaySettings_->isEnabled()) {
            // Retard the decay mask by 1 degree to hide previous data.
            //
            glPushMatrix();
            glRotatef(lastAzimuth_ - 1.0, 0.0f, 0.0f, -1.0f);
            GLEC(glCallList(getDisplayList(kDecay)));
            glPopMatrix();
        } else {
            double sine, cosine;
            sineCosineLUT_->lookup(lastShaftEncoding_, sine, cosine);
            glColor4f(1.0, 1.0, 1.0, 0.5);
            glLineWidth(1.0);
            glBegin(GL_LINES);
            glVertex2f(0.0, 0.0);
            glVertex2f(sine * viewSettings_->getRangeMax(), cosine * viewSettings_->getRangeMax());
            glEnd();
        }
    }

    if (backgroundTexture_ && backgroundImageSettings_->isEnabled())
        GLEC(glCallList(getDisplayList(kRenderBackgroundTexture)));

    // The following overlays use the stencil buffer to keep from drawing multiple times over the same spot.
    // Without this protection, pixels drawn with an alpha value < 1.0 will have different alpha values
    // depending on how many times they were drawn on by a feature. Note that for now, the buffer is cleared for
    // each overlay.
    //
    {
        StencilBufferState stencilState;
        if (rangeMapImaging_->isEnabled()) {
            stencilState.use();
            glCallList(getDisplayList(kRangeMap));
        }

        if (rangeRingsImaging_->isEnabled()) {
            stencilState.use();
            glCallList(getDisplayList(kRangeRings));
        }
    }

    if (extractionsImaging_->isEnabled()) { drawExtractions(widget); }

    if (rangeTruthsImaging_->isEnabled()) { drawRangeTruths(widget); }

    if (bugPlotsImaging_->isEnabled()) { drawBugPlots(widget); }

    glDisable(GL_BLEND);
}

void
PPIWidget::remakeDecayTexture()
{
    makeCurrent();
    makeDecayTexture();
}

void
PPIWidget::remakeDesaturationTexture()
{
    Logger::ProcLog log("remakeDesaturationTexture", Log());
    LOGINFO << std::endl;
    makeCurrent();
    makeDesaturationTexture();
    makeDesaturationList();
    redisplayVideo();
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
    glGetIntegerv(GL_VIEWPORT, viewPort_);
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

    xSpan_ = viewSettings_->getZoom() * viewSettings_->getRangeMax();
    ySpan_ = xSpan_;

    if (width() > height()) {
        xSpan_ = width() / 2.0 * xSpan_ * 2.0 / height();
    } else if (width() < height()) {
        ySpan_ = height() / 2.0 * ySpan_ * 2.0 / width();
    }

    xScale_ = xSpan_ * 2.0 / width();
    yScale_ = ySpan_ * 2.0 / height();

    glLoadIdentity();
    glOrtho(viewSettings_->getX() - xSpan_, viewSettings_->getX() + xSpan_, viewSettings_->getY() - ySpan_,
            viewSettings_->getY() + ySpan_, -1.0, 1.0);

    // Fetch the projection matrix components so we can map the cursor position to real-world.
    //
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix_);

    makeRangeRingsList();
    makeRangeMapList();
}

void
PPIWidget::viewSettingsChanged()
{
    makeCurrent();
    setViewTransform();
}

void
PPIWidget::historyCurrentViewChanged(int age)
{
    static Logger::ProcLog log("historyCurrentViewChanged", Log());
    LOGINFO << "age: " << age << std::endl;
    const History::Entry& entry(history_->getViewedEntry());
    if (!entry.getVideo().empty()) {
        info_ = entry.getVideo().back();
        lastAzimuth_ = radiansToDegrees(info_->getAzimuthStart());
        lastShaftEncoding_ = info_->getShaftEncoding();
    }

    repaintVideo(entry.getVideo());
    repaintBinary(entry.getBinary());

    if (age) {
        showMessage(QString("Viewing past rotation #%2.").arg(age));
    } else {
        showMessage("Viewing current rotation.");
    }
}

void
PPIWidget::historyCurrentViewAged(int age)
{
    static Logger::ProcLog log("historyCurrentViewAged", Log());
    LOGINFO << "age: " << age << std::endl;
    showMessage(QString("Viewing past rotation #%2.").arg(age));
}

void
PPIWidget::redisplayVideo()
{
    const History::Entry& entry(history_->getViewedEntry());
    repaintVideo(entry.getVideo());
}

void
PPIWidget::redisplayBinary()
{
    const History::Entry& entry(history_->getViewedEntry());
    repaintBinary(entry.getBinary());
}

void
PPIWidget::clearAll()
{
    clearVideoBuffer();
    clearBinaryBuffer();
    history_->clearAll();
}

void
PPIWidget::clearVideoBuffer()
{
    makeCurrent();
    videoVertexGenerator_->flushAll();
    if (videoBuffer_) videoBuffer_->clearBuffer();
}

void
PPIWidget::clearBinaryBuffer()
{
    makeCurrent();
    binaryVertexGenerator_->flushAll();
    if (binaryBuffer_) binaryBuffer_->clearBuffer();
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
PPIWidget::pan(double xf, double yf)
{
    viewSettings_->shift(xSpan_ * xf, ySpan_ * yf);
}

void
PPIWidget::changeZoom(int change)
{
    viewSettings_->setZoomPower(viewSettings_->getZoomPower() + change);
}

void
PPIWidget::keyPressEvent(QKeyEvent* event)
{
    if (rubberBanding_) {
        if (event->key() == Qt::Key_Escape) {
            event->accept();
            rubberBanding_ = false;
        }
    } else {
        if (event->key() == Qt::Key_Shift) {
            setCursor(Qt::OpenHandCursor);
            setToolTip("");
            setToolTip(cursorPosition_);
        } else {
            setCursor(Qt::CrossCursor);
        }
    }

    Super::keyPressEvent(event);
}

void
PPIWidget::keyReleaseEvent(QKeyEvent* event)
{
    setCursor(Qt::CrossCursor);
    Super::keyPressEvent(event);
}

void
PPIWidget::enterEvent(QEvent* event)
{
    event->accept();
    setCursor(Qt::CrossCursor);
    Super::enterEvent(event);
}

void
PPIWidget::leaveEvent(QEvent* event)
{
    phantomCursor_ = PhantomCursorImaging::InvalidCursor();
    Super::leaveEvent(event);
    if (rubberBanding_) { rubberBanding_ = false; }
}

void
PPIWidget::mousePressEvent(QMouseEvent* event)
{
    Logger::ProcLog log("mousePressEvent", Log());
    LOGTIN << "button: " << event->button() << std::endl;
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() == 0) {
            event->accept();
            magStartPos_ = event->pos();
            localToRealWorld(event->x(), event->y(), magStartX_, magStartY_);
            magEndX_ = magStartX_;
            magEndY_ = magStartY_;
            rubberBanding_ = true;
        } else if (event->modifiers() == Qt::ShiftModifier) {
            event->accept();
            setCursor(Qt::ClosedHandCursor);
            panFrom_ = event->pos();
            panning_ = true;
        } else if (event->modifiers() == Qt::ControlModifier) {
            event->accept();
            GLdouble objX, objY;
            localToRealWorld(event->x(), event->y(), objX, objY);
            double azimuth = Utils::normalizeRadians(::atan2(objX, objY));
            double range = ::sqrt(objX * objX + objY * objY);
            Messages::BugPlot::Ref msg = bugPlotEmitterSettings_->addBugPlot(range, azimuth);
            LOGDEBUG << "az: " << azimuth << " range: " << range << " msg: " << (msg ? 'Y' : 'N') << std::endl;
            if (msg) { history_->addBugPlot(msg); }
        }
    }
    Super::mousePressEvent(event);
}

void
PPIWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (panning_) {
        setCursor(Qt::ClosedHandCursor);
        GLdouble x, y;
        localToRealWorld(panFrom_.x(), panFrom_.y(), x, y);
        QPointF f(x, y);
        localToRealWorld(event->x(), event->y(), x, y);
        QPointF t(x, y);
        QRectF fromTo(QRectF(f, QSizeF(t.x() - f.x(), t.y() - f.y())));
        viewSettings_->shift(-fromTo.width(), -fromTo.height());
        panFrom_ = event->pos();
    } else if (rubberBanding_) {
        localToRealWorld(event->x(), event->y(), magEndX_, magEndY_);
    }
}

void
PPIWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (rubberBanding_) {
            event->accept();
            rubberBanding_ = false;
            if (std::abs(magStartPos_.x() - event->x()) > 20 && std::abs(magStartPos_.y() - event->y()) > 20) {
                MagnifierWindow* magnifier = new MagnifierWindow(this);
                magnifier->initialize();
                localToRealWorld(event->x(), event->y(), magEndX_, magEndY_);
                magnifier->setBounds(std::min(magStartX_, magEndX_), std::min(magStartY_, magEndY_),
                                     std::abs(magStartX_ - magEndX_), std::abs(magStartY_ - magEndY_));
                magnifier->showAndRaise();
            }
        } else if (panning_) {
            panning_ = false;
            if (event->modifiers() == Qt::ShiftModifier)
                setCursor(Qt::OpenHandCursor);
            else
                setCursor(Qt::CrossCursor);
        }
    }
    Super::mouseReleaseEvent(event);
}

void
PPIWidget::wheelEvent(QWheelEvent* event)
{
    static Logger::ProcLog log("wheelEvent", Log());
    LOGINFO << event->delta() << std::endl;
    event->accept();

    // The delta() value is given in 1/8ths of a degree. Apparently the standard for the scroll wheel is to
    // report in 15 degree steps. Thus the division by 120.
    //
    int steps = event->delta() / 120;
    changeZoom(steps);
    Super::wheelEvent(event);
}

void
PPIWidget::localToRealWorld(int inX, int inY, GLdouble& outX, GLdouble& outY) const
{
    double winX = inX;
    double winY = viewPort_[3] - inY;
    double winZ = 0.0;
    double outZ = 0.0;
    UnProjectPoint(winX, winY, winZ, modelViewMatrix_, projectionMatrix_, viewPort_, &outX, &outY, &outZ);
}

void
PPIWidget::updateCursorPosition(CursorPosition& pos)
{
    if (history_) {
        bool isValid = false;

        if (videoImaging_->isEnabled()) {
            Messages::Video::DatumType datum = history_->getVideoValue(pos.getAzimuth(), pos.getRange(), isValid);
            if (isValid) pos.setSampleValue(QString::number(datum));
        } else if (binaryImaging_->isEnabled()) {
            Messages::BinaryVideo::DatumType datum =
                history_->getBinaryValue(pos.getAzimuth(), pos.getRange(), isValid);
            if (isValid) pos.setSampleValue(datum ? "T" : "F");
        }
    }
}

void
PPIWidget::showCursorInfo()
{
    GLdouble objX, objY;
    localToRealWorld(mouse_.x(), mouse_.y(), objX, objY);

    CursorPosition pos(objX, objY);
    updateCursorPosition(pos);
    setCursorPosition(pos.getToolTip());

    emit currentCursorPosition(pos.getXY());
}

void
PPIWidget::centerAtCursor()
{
    // Calculate the real-world position for the cursor position, then center the view with the result.
    //
    QPoint pos(mapFromGlobal(QCursor::pos()));
    GLdouble x, y;
    localToRealWorld(pos.x(), pos.y(), x, y);
    viewSettings_->moveTo(x, y);

    // Move the mouse cursor to the center of the widget.
    //
    pos = QPoint(width() / 2, height() / 2);
    QCursor::setPos(mapToGlobal(pos));
}

void
PPIWidget::makeBackgroundTexture(const QImage& image)
{
    Logger::ProcLog log("makeBackgroundTexture", Log());

    makeCurrent();
    if (backgroundTexture_) {
        deleteTexture(backgroundTexture_.getId());
        backgroundTexture_ = Texture();
    }

    if (!image.isNull()) {
        GLenum textureType = Texture::GetBestTextureType();
        GLuint id = bindTexture(image, textureType);
        backgroundTexture_ = Texture(textureType, id, image.width(), image.height());
        makeRenderTextureList(kRenderBackgroundTexture, backgroundTexture_);
    }
}

void
PPIWidget::showMessage(const QString& text, int duration) const
{
    MainWindow* mainWindow = qobject_cast<MainWindow*>(window());
    if (mainWindow) { mainWindow->statusBar()->showMessage(text, duration); }
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

    update();

    if (underMouse()) {
        phantomCursor_ = phantomCursorImaging_->InvalidCursor();
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
    if (!updateTimer_.isActive()) updateTimer_.start(kUpdateRate, this);
    Super::showEvent(event);
}

void
PPIWidget::closeEvent(QCloseEvent* event)
{
    Logger::ProcLog log("closeEvent", Log());
    LOGINFO << std::endl;
    if (updateTimer_.isActive()) updateTimer_.stop();
    Super::closeEvent(event);
}

void
PPIWidget::videoChannelChanged(const QString& name)
{
    if (name.size()) {
        history_->clearVideo();
        clearVideoBuffer();
    }
}

void
PPIWidget::binaryChannelChanged(const QString& name)
{
    if (name.size()) {
        history_->clearBinary();
        clearBinaryBuffer();
    }
}

void
PPIWidget::extractionsChannelChanged(const QString& name)
{
    if (name.size()) { history_->clearExtractions(); }
}

void
PPIWidget::rangeTruthsChannelChanged(const QString& name)
{
    if (name.size()) { history_->clearRangeTruths(); }
}

void
PPIWidget::bugPlotsChannelChanged(const QString& name)
{
    if (name.size()) { history_->clearBugPlots(); }
}

void
PPIWidget::setPhantomCursor(const QPointF& pos)
{
    if (!underMouse())
        phantomCursor_ = pos;
    else
        phantomCursor_ = PhantomCursorImaging::InvalidCursor();
}

void
PPIWidget::addMagnifier(MagnifierWindow* magnifier)
{
    magnifiers_.append(magnifier);
}

void
PPIWidget::removeMagnifier(MagnifierWindow* magnifier)
{
    if (!magnifier) magnifier = static_cast<MagnifierWindow*>(sender());
    magnifiers_.removeAll(magnifier);
    phantomCursor_ = phantomCursorImaging_->InvalidCursor();
    update();
}

void
PPIWidget::saveMagnifiers(QSettings& settings) const
{
    Logger::ProcLog log("saveMagnifiers", Log());
    LOGINFO << "count: " << magnifiers_.size() << " group: " << settings.group() << std::endl;
    settings.beginWriteArray("Magnifiers", magnifiers_.size());
    for (int index = 0; index < magnifiers_.size(); ++index) {
        settings.setArrayIndex(index);
        magnifiers_[index]->save(settings);
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
    foreach (MagnifierWindow* mag, magnifiers_)
        mag->showAndRaise();
}

void
PPIWidget::setShowCursorPosition(bool state)
{
    showCursorPosition_ = state;
    setToolTip("");
    if (state) setToolTip(cursorPosition_);
}

void
PPIWidget::setCursorPosition(const QString& value)
{
    if (value != cursorPosition_) {
        cursorPosition_ = value;
        if (showCursorPosition_) setToolTip(cursorPosition_);
    } else {
        setToolTip("");
        setToolTip(value);
    }
}
