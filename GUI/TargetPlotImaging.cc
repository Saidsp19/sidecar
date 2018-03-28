#include "QtGui/QComboBox"
#include "QtGui/QPainter"
#include "QtGui/QPixmap"

#include "GUI/Utils.h"
#include "Utils/Utils.h"

#include "GLFont.h"
#include "LogUtils.h"
#include "PlotSymbolWidget.h"
#include "TargetPlotImaging.h"
#include "VertexColorTagArray.h"

using namespace SideCar::GUI;

PlotPositionFunctor::PlotPositionFunctor() : font_(0)
{
    ;
}

PlotPositionFunctor::~PlotPositionFunctor()
{
    delete font_;
}

void
PlotPositionFunctor::renderTags(int tagSize, const VertexColorTagArray& tags)
{
    if (!font_ || font_->getFont().pointSize() != tagSize) {
        delete font_;
        font_ = 0;
        QFont font;
        if (font.pointSize() != tagSize) font.setPointSize(tagSize);
        font_ = new GLFont(font);
    }

    font_->render(tags);
}

static const QPoint kDiamondPoints_[] = {QPoint(-10, 0), QPoint(0, 10), QPoint(10, 0), QPoint(0, -10)};

static const QPoint kSquarePoints_[] = {QPoint(-10, 10), QPoint(10, 10), QPoint(10, -10), QPoint(-10, -10)};

static const QPoint kTrianglePoints_[] = {QPoint(0, 10), QPoint(-10, -10), QPoint(10, -10)};

static const QPoint kDaggerPoints_[] = {QPoint(-10, 10), QPoint(10, 10), QPoint(0, -10)};

static const QPoint kCrossPoints_[] = {
    QPoint(-10, -10),
    QPoint(10, 10),
    QPoint(-10, 10),
    QPoint(10, -10),
};

static const QPoint kSquareCrossPoints_[] = {QPoint(-10, -10), QPoint(10, 10),   QPoint(-10, 10),  QPoint(10, -10),
                                             QPoint(10, -10),  QPoint(-10, -10), QPoint(-10, -10), QPoint(-10, 10),
                                             QPoint(-10, 10),  QPoint(10, 10),   QPoint(10, 10),   QPoint(10, -10)};

static const QPoint kPlusPoints_[] = {
    QPoint(-10, 0),
    QPoint(10, 0),
    QPoint(0, -10),
    QPoint(0, 10),
};

static const QPoint kDiamondPlusPoints_[] = {QPoint(-10, 0), QPoint(10, 0),  QPoint(0, -10), QPoint(0, 10),
                                             QPoint(0, 10),  QPoint(10, 0),  QPoint(10, 0),  QPoint(0, -10),
                                             QPoint(0, -10), QPoint(-10, 0), QPoint(-10, 0), QPoint(0, 10)};

template <typename T>
static size_t
ArraySize(const T& a)
{
    return sizeof(a) / sizeof(QPoint);
}

TargetPlotImaging::SymbolInfo TargetPlotImaging::kSymbolInfo_[] = {
    {"Cross", GL_LINES, kCrossPoints_, ArraySize(kCrossPoints_)},
    {"Cross + Square", GL_LINES, kSquareCrossPoints_, ArraySize(kSquareCrossPoints_)},
    {"Diamond (Filled)", GL_QUADS, kDiamondPoints_, ArraySize(kDiamondPoints_)},
    {"Diamond (Hollow)", GL_LINE_LOOP, kDiamondPoints_, ArraySize(kDiamondPoints_)},
    {"Plus", GL_LINES, kPlusPoints_, ArraySize(kPlusPoints_)},
    {"Plus + Diamond", GL_LINES, kDiamondPlusPoints_, ArraySize(kDiamondPlusPoints_)},
    {"Square (Filled)", GL_QUADS, kSquarePoints_, ArraySize(kSquarePoints_)},
    {"Square (Hollow)", GL_LINE_LOOP, kSquarePoints_, ArraySize(kSquarePoints_)},
    {"Triangle (Filled)", GL_TRIANGLES, kTrianglePoints_, ArraySize(kTrianglePoints_)},
    {"Triangle (Hollow)", GL_LINE_LOOP, kTrianglePoints_, ArraySize(kTrianglePoints_)},
    {"Dagger (Filled)", GL_TRIANGLES, kDaggerPoints_, ArraySize(kDaggerPoints_)},
    {"Dagger (Hollow)", GL_LINE_LOOP, kDaggerPoints_, ArraySize(kDaggerPoints_)}};

const TargetPlotImaging::SymbolInfo&
TargetPlotImaging::GetSymbolInfo(SymbolType symbolType)
{
    return kSymbolInfo_[symbolType];
}

const QString&
TargetPlotImaging::GetSymbolName(SymbolType symbolType)
{
    return GetSymbolInfo(symbolType).name;
}

Logger::Log&
TargetPlotImaging::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.TargetPlotImaging");
    return log_;
}

TargetPlotImaging::TargetPlotImaging(PlotPositionFunctor* plotPositionFunctor, BoolSetting* visible,
                                     ColorButtonSetting* color, DoubleSetting* extent, OpacitySetting* opacity,
                                     QComboBoxSetting* symbolType, DoubleSetting* lineWidth, IntSetting* lifeTime,
                                     BoolSetting* fadeEnabled, BoolSetting* showTrails, IntSetting* tagSize,
                                     BoolSetting* showTags) :
    ChannelImaging(visible, color, extent, opacity),
    plotPositionFunctor_(plotPositionFunctor), symbolType_(symbolType), lineWidth_(lineWidth), lifeTime_(lifeTime),
    fadeEnabled_(fadeEnabled), showTrails_(showTrails), tagSize_(tagSize), showTags_(showTags), points_(),
    symbolPoints_()
{
    add(symbolType);
    add(lineWidth);
    add(lifeTime);
    add(fadeEnabled);

    if (showTrails) add(showTrails);

    if (showTags) add(showTags);

    connect(extent, SIGNAL(valueChanged(double)), SLOT(clearSymbolPoints()));
    connect(symbolType, SIGNAL(valueChanged(int)), SLOT(clearSymbolPoints()));
    connect(lifeTime, SIGNAL(valueChanged(int)), SLOT(postLifeTimeChanged(int)));
}

void
TargetPlotImaging::clearSymbolPoints()
{
    symbolPoints_.clear();
}

void
TargetPlotImaging::sizeChanged(double size)
{
    clearSymbolPoints();
    Super::sizeChanged(size);
}

void
TargetPlotImaging::postLifeTimeChanged(int value)
{
    postSettingChanged();
    emit lifeTimeChanged(value * 1000);
}

void
TargetPlotImaging::scaleSymbolPoints()
{
    static Logger::ProcLog log("scaleSymbolPoints", Log());

    const SymbolInfo& symbolInfo(GetSymbolInfo(getSymbolType()));

    symbolPoints_.clear();
    GLfloat size = getExtent2() / 10.0;
    for (size_t index = 0; index < symbolInfo.size; ++index) {
        symbolPoints_.push_back(Vertex(symbolInfo.points[index].x() * size, symbolInfo.points[index].y() * size));
    }
}

void
TargetPlotImaging::render(QWidget* widget, const TargetPlotList& targets)
{
    static Logger::ProcLog log("render", Log());

    if (!plotPositionFunctor_) return;

    // Fetch the matrices involved in converting from model coordinates into window positions. This should be
    // very light-weight, but if not, then create a new class that contains these values and provide them as an
    // input into the render() method. We cannot cache these values within this class because it may be used by
    // more than one view.
    //
    GLdouble modelMatrix[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    GLdouble projectionMatrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
    GLint viewPort[4];
    glGetIntegerv(GL_VIEWPORT, viewPort);

    // All of our points window coordinates. Save the current transformation matrix and set up an identity
    // transform.
    //
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // gluOrtho2D(0, widget->width(), 0, widget->height());
    glOrtho(0, widget->width(), 0, widget->height(), -1.0, 1.0);
    glLineWidth(getLineWidth());

    if (symbolPoints_.empty()) scaleSymbolPoints();

    int kind = GetSymbolInfo(getSymbolType()).kind;
    VertexColorTagArray tags;

    // Process oldest target first so that newer targets are not obscured by the older ones.
    //
    for (int targetIndex = 0; targetIndex < targets.size(); ++targetIndex) {
        const TargetPlot& target(targets[targetIndex]);

        // Obtained target plot color, faded by time if enabled.
        //
        Color color(getColor());
        if (getFadeEnabled()) fadeColor(color, target.getAge());

        // Calculate the position of the target in window coordinates. This allows us to draw the symbols at the
        // same size, regardless of the view settings.
        //
        Vertex center = plotPositionFunctor_->getPosition(target);
        GLdouble x = center.x;
        GLdouble y = center.y;

        GLdouble winX, winY, winZ;
        // gluProject(x, y, 0.0, modelMatrix, projectionMatrix, viewPort,
        //&winX, &winY, &winZ);
        ProjectPoint(x, y, 0.0, modelMatrix, projectionMatrix, viewPort, &winX, &winY, &winZ);
        LOGDEBUG << "xy: " << x << ' ' << y << " winXY: " << winX << ' ' << winY << std::endl;

        center.x = winX;
        center.y = winY;
        for (size_t symbolIndex = 0; symbolIndex < symbolPoints_.size(); ++symbolIndex) {
            points_.push_back(symbolPoints_[symbolIndex] + center, color);
        }

        // Due to the definition for GL_LINE_LOOP, we cannot batch them up in the points_ vector and draw them
        // all at once. We must do each individually.
        //
        if (kind == GL_LINE_LOOP) {
            points_.draw(kind);
            points_.clear();
        }

        // Draw the target's alphanumeric tag if available and enabled.
        //
        if (showTags_ && showTags_->getValue() && target.getTag().size()) {
            center.x += getExtent2();
            center.y += getExtent2();
            tags.push_back(center, color, target.getTag());
            LOGDEBUG << "tag: " << target.getTag() << std::endl;
        }
    }

    // Draw the plot symbols.
    //
    if (!points_.empty()) {
        points_.draw(kind);
        points_.clear();
    }

    // Now draw any tags.
    //
    if (!tags.empty()) plotPositionFunctor_->renderTags(tagSize_->getValue(), tags);

    glPopMatrix();
}

void
TargetPlotImaging::render(QWidget* widget, const TargetPlotListList& targets)
{
    static Logger::ProcLog log("render", Log());

    if (!plotPositionFunctor_) return;

    // Fetch the matrices involved in converting from model coordinates into window positions. This should be
    // very light-weight, but if not, then create a new class that contains these values and provide them as an
    // input into the render() method. We cannot cache these values within this class because it may be used by
    // more than one view.
    //
    GLdouble modelMatrix[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    GLdouble projectionMatrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
    GLint viewPort[4];
    glGetIntegerv(GL_VIEWPORT, viewPort);

    // All of our points window coordinates. Save the current transformation matrix and set up an identity
    // transform.
    //
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // gluOrtho2D(0, widget->width(), 0, widget->height());
    glOrtho(0, widget->width(), 0, widget->height(), -1.0, 1.0);
    glLineWidth(getLineWidth());

    if (symbolPoints_.empty()) scaleSymbolPoints();

    int kind = GetSymbolInfo(getSymbolType()).kind;
    VertexColorTagArray tags;
    VertexColorArray lines;

    // Process oldest target first so that newer targets are not obscured by the older ones.
    //
    for (int targetIndex = 0; targetIndex < targets.size(); ++targetIndex) {
        const TargetPlotList& plots(targets[targetIndex]);

        int plotIndex = plots.size() - 1;
        if (!showTrails_ || !showTrails_->getValue()) plotIndex = 0;

        for (; plotIndex >= 0; --plotIndex) {
            const TargetPlot& target(plots[plotIndex]);

            // Obtained target plot color, faded by time if enabled.
            //
            Color color(getColor());
            if (getFadeEnabled()) fadeColor(color, target.getAge());

            // Calculate the position of the target in window coordinates. This allows us to draw the symbols at
            // the same size, regardless of the view settings.
            //
            Vertex center = plotPositionFunctor_->getPosition(target);
            GLdouble x = center.x;
            GLdouble y = center.y;

            GLdouble winX, winY, winZ;
            // gluProject(x, y, 0.0, modelMatrix, projectionMatrix, viewPort,
            //        &winX, &winY, &winZ);
            ProjectPoint(x, y, 0.0, modelMatrix, projectionMatrix, viewPort, &winX, &winY, &winZ);
            LOGDEBUG << "xy: " << x << ' ' << y << " winXY: " << winX << ' ' << winY << std::endl;

            center.x = winX;
            center.y = winY;

            lines.push_back(center, color);

            for (size_t symbolIndex = 0; symbolIndex < symbolPoints_.size(); ++symbolIndex) {
                points_.push_back(symbolPoints_[symbolIndex] + center, color);
            }

            // Due to the definition for GL_LINE_LOOP, we cannot batch them up in the points_ vector and draw
            // them all at once. We must do each individually.
            //
            if (kind == GL_LINE_LOOP) {
                points_.draw(kind);
                points_.clear();
            }

            // Draw the target's alphanumeric tag if available and enabled.
            //
            if (plotIndex == 0 && showTags_ && showTags_->getValue() && target.getTag().size()) {
                center.x += getExtent2();
                center.y += getExtent2();
                tags.push_back(center, color, target.getTag());
                LOGDEBUG << "tag: " << target.getTag() << std::endl;
            }
        }

        if (!lines.empty()) {
            if (lines.size() > 1) { lines.draw(GL_LINE_STRIP); }
            lines.clear();
        }
    }

    // Draw the plot symbols.
    //
    if (!points_.empty()) {
        points_.draw(kind);
        points_.clear();
    }

    // Now draw any tags.
    //
    if (!tags.empty()) plotPositionFunctor_->renderTags(tagSize_->getValue(), tags);

    glPopMatrix();
}

void
TargetPlotImaging::connectPlotSymbolWidget(PlotSymbolWidget* widget)
{
    addSymbolIcons(widget);
    symbolType_->connectWidget(widget);
}

void
TargetPlotImaging::addSymbolIcons(QComboBox* widget) const
{
    widget->clear();

    for (int index = 0; index < TargetPlotImaging::kNumSymbolTypes; ++index) {
        TargetPlotImaging::SymbolInfo& symbolInfo = kSymbolInfo_[index];

        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::black);

        QPainter painter;
        painter.begin(&pixmap);

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setWindow(QRect(-12, -12, 24, 24));
        painter.setPen(QPen(getQColor(), 1.0));
        painter.setBrush(Qt::NoBrush);

        // Flip vertically because Qt and OpenGL have different ways of counting in the Y direction.
        //
        painter.rotate(180);

        switch (symbolInfo.kind) {
        case GL_LINES: painter.drawLines(symbolInfo.points, symbolInfo.size / 2); break;

        case GL_TRIANGLES:
        case GL_QUADS:
            painter.setBrush(getQColor());
            painter.drawPolygon(symbolInfo.points, symbolInfo.size);
            break;

        case GL_LINE_LOOP: painter.drawPolygon(symbolInfo.points, symbolInfo.size); break;

        default: break;
        }

        painter.end();

        widget->addItem(QIcon(pixmap), QString());
    }

    widget->setCurrentIndex(symbolType_->getValue());
}
