#include <cmath>

#include "QtGui/QPainter"
#include "QtOpenGL/QGLWidget"

#include "LogUtils.h"
#include "PhantomCursorImaging.h"

using namespace SideCar::GUI;

Logger::Log&
PhantomCursorImaging::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.PhantomCursorImaging");
    return log_;
}

PhantomCursorImaging::PhantomCursorImaging(BoolSetting* enabled, ColorButtonSetting* color, DoubleSetting* size,
                                           OpacitySetting* opacity, IntSetting* radius) :
    ChannelImaging(enabled, color, size, opacity),
    radius_(radius)
{
    add(radius);
}

void
PhantomCursorImaging::drawCursor(QPainter& painter, const QPointF& pos) const
{
    if (isEnabled() && IsValidCursor(pos)) {
        painter.setRenderHints(QPainter::Antialiasing);
        painter.setPen(QPen(getQColor(), getSize()));
        painter.setOpacity(getAlpha());
        double radius = getRadius();

        double x = pos.x() * painter.device()->width();
        double y = (1.0 - pos.y()) * painter.device()->height();

        painter.drawLine(QLineF(x - radius, y, x + radius, y));
        painter.drawLine(QLineF(x, y - radius, x, y + radius));
#if 0
    painter.drawEllipse(QRectF(pos.x() * painter.device()->width() -
                               radius,
                               (1.0 - pos.y()) *
                               painter.device()->height() - radius,
                               radius * 2.0, radius * 2.0));
#endif
    }
}

void
PhantomCursorImaging::drawCursor(const QPointF& pos, double xScale, double yScale) const
{
    static Logger::ProcLog log("drawCursor", Log());
    LOGINFO << "pos: " << pos << " enabled: " << isEnabled() << " isValid: " << IsValidCursor(pos) << std::endl;

    if (isEnabled() && IsValidCursor(pos)) {
        glLineWidth(getSize());
        getColor().use();
        glBegin(GL_LINES);
        float xSpan = xScale * getRadius();
        float ySpan = yScale * getRadius();
        glVertex2f(pos.x() - xSpan, pos.y());
        glVertex2f(pos.x() + xSpan, pos.y());
        glVertex2f(pos.x(), pos.y() + ySpan);
        glVertex2f(pos.x(), pos.y() - ySpan);
        glEnd();
    }
}
