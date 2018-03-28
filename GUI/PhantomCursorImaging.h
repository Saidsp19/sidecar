#ifndef SIDECAR_GUI_PHANTOMCURSORIMAGING_H // -*- C++ -*-
#define SIDECAR_GUI_PHANTOMCURSORIMAGING_H

#include <limits>

#include "GUI/ChannelImaging.h"
#include "GUI/IntSetting.h"

class QPainter;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class PhantomCursorImaging : public ChannelImaging {
public:
    static QPointF InvalidCursor()
    {
        return QPointF(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    }

    static bool IsInvalidCursor(const QPointF& pos) { return pos.y() == std::numeric_limits<double>::max(); }

    static bool IsValidCursor(const QPointF& pos) { return !IsInvalidCursor(pos); }

    static Logger::Log& Log();

    PhantomCursorImaging(BoolSetting* enabled, ColorButtonSetting* color, DoubleSetting* size, OpacitySetting* opacity,
                         IntSetting* radius);

    int getRadius() const { return radius_->getValue(); }

    void drawCursor(QPainter& painter, const QPointF& pos) const;

    void drawCursor(const QPointF& pos, double xScale, double yScale) const;

private:
    IntSetting* radius_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
