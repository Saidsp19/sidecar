#include <algorithm>
#include <cmath>
#include <functional>

#include "QtGui/QGuiApplication"
#include "QtGui/QPainter"

#include "GUI/LogUtils.h"
#include "GUI/Utils.h"
#include "Utils/Utils.h"

#include "ScaleWidget.h"

using namespace SideCar::GUI::BScope;

Logger::Log&
ScaleWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.ScaleWidget");
    return log_;
}

ScaleWidget::ScaleWidget(QWidget* parent, Qt::Orientation orientation) :
    Super(parent), lastSizeCalculated_(), majorTickHeight_(10), minorTickHeight_(6), majorTickDivisionsMax_(8),
    minorTickDivisionsMax_(4), start_(0.0), range_(4.0), orientation_(orientation),
    majorGridPen_(QColor(64, 64, 64), 0.0), minorGridPen_(QColor(16, 16, 16), 0.0),
    cursorIndicatorPen_(QColor(255, 0, 0, 128), 0), cursorPosition_(-1), autoDivide_(false)
{
    QFont labelFont(font());
    labelFont.setPointSize(10);
    setFont(labelFont);
    setBackgroundRole(QPalette::Base);
    setOrientation(orientation);
}

void
ScaleWidget::setSpan(int span)
{
    static Logger::ProcLog log("setSpan", Log());
    LOGINFO << "span: " << span << std::endl;

    if (orientation_ == Qt::Horizontal) {
        resize(span, height());
    } else {
        resize(width(), span);
    }

    updateGeometry();
    recalculateTickIntervals(width(), height());
}

QSize
ScaleWidget::sizeHint() const
{
    if (orientation_ == Qt::Horizontal)
        return QSize(width(), majorTickHeight_ + fontMetrics().height());
    else
        return QSize(majorTickHeight_ + fontMetrics().height(), height());
}

QSize
ScaleWidget::minimumSizeHint() const
{
    return sizeHint();
}

void
ScaleWidget::recalculateTickIntervals(int width, int height)
{
    QSize newSize(width, height);
    if (newSize == lastSizeCalculated_) return;

    lastSizeCalculated_ = newSize;
    double span = (orientation_ == Qt::Horizontal ? width : height);

    if (autoDivide_) {
        majorIncrement_ = std::max(80.0, span / majorTickDivisionsMax_);
        majorTickDivisions_ = std::max(1, int(::floor(span / majorIncrement_)));
        majorIncrement_ = span / majorTickDivisions_;
        minorIncrement_ = std::max(40.0, majorIncrement_ / minorTickDivisionsMax_);
        minorTickDivisions_ = std::max(1, int(::floor(majorIncrement_ / minorIncrement_)));
        minorIncrement_ = majorIncrement_ / minorTickDivisions_;
    } else {
        majorIncrement_ = span / majorTickDivisionsMax_;
        majorTickDivisions_ = majorTickDivisionsMax_;
        minorIncrement_ = majorIncrement_ / minorTickDivisionsMax_;
        minorTickDivisions_ = minorTickDivisionsMax_;
    }
}

void
ScaleWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    render(painter);
    if (cursorPosition_ != -1) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        drawCursorPosition(painter, cursorPosition_);
    }
}

void
ScaleWidget::render(QPainter& painter)
{
    render(painter, width(), height());
}

void
ScaleWidget::render(QPainter& painter, int width, int height)
{
    recalculateTickIntervals(width, height);

    // For horizontal and vertical lines, disable antialiasing.
    //
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    if (orientation_ == Qt::Horizontal) {
        drawHorizontalTicks(painter, width, height);
    } else {
        painter.save();
        painter.translate(width, 0.0);
        painter.rotate(+90.0);
        drawVerticalTicks(painter, height, width);
        painter.restore();
    }
}

void
ScaleWidget::drawVerticalTicks(QPainter& painter, int span, int tagOffset)
{
    static Logger::ProcLog log("drawVerticalTicks", Log());
    LOGINFO << "span: " << span << " tagOffset: " << tagOffset << std::endl;

    auto palette = QGuiApplication::palette();
    painter.setFont(font());
    painter.setBrush(palette.windowText());
    QFontMetrics fm(fontMetrics());

    double increment = minorIncrement_;
    double origin = 0.0;
    double tag = start_;
    int tickCounter = 0;
    double tagIncrement = range_ / majorTickDivisions_;

    if (orientation_ == Qt::Vertical) {
        increment = -increment;
        origin = span - 1;
    }

    while (1) {
        double x = tickCounter * increment + origin;

        LOGDEBUG << "x: " << x << " tickCounter: " << tickCounter << std::endl;

        if (x >= span || x < 0) break;

        if ((tickCounter % minorTickDivisions_) == 0) {
            QLineF line(x, 0.0, x, majorTickHeight_);
            painter.drawLine(line);
            QString label = formatTickTag(tag);
            double offset = fm.boundingRect(label).width() / 2;

            if (orientation_ == Qt::Horizontal) {
                if (tickCounter == 0) {
                    offset = 0;
                } else if (x + offset > span) {
                    offset *= 2;
                }
            } else {
                if (tickCounter == 0) {
                    offset *= 2;
                } else if (x - offset < 0.0) {
                    offset = 0;
                }
            }

            LOGDEBUG << "tag: " << tag << " offset: " << offset << std::endl;

            line.translate(-offset, tagOffset - fm.descent() - 1);
            painter.drawText(line.p1(), label);
            tag += tagIncrement;
        } else {
            QLineF line(x, 0.0, x, minorTickHeight_);
            painter.drawLine(line);
        }

        ++tickCounter;
    }
}

void
ScaleWidget::drawHorizontalTicks(QPainter& painter, int span, int tagOffset)
{
    static Logger::ProcLog log("drawHorizontalTicks", Log());
    LOGINFO << "span: " << span << " tagOffset: " << tagOffset << std::endl;

    auto palette = QGuiApplication::palette();
    painter.setFont(font());
    painter.setBrush(palette.windowText());
    QFontMetrics fm(fontMetrics());

    double tag = 0.0;
    int tickCounter = 0;
    double tagIncrement = range_ / majorTickDivisions_;
    double shift = -start_ / range_ * (span - 1);

    LOGDEBUG << "tagIncrement: " << tagIncrement << std::endl;

    while (1) {
        double x = shift - tickCounter * minorIncrement_;
        LOGDEBUG << "x: " << x << " tickCounter: " << tickCounter << std::endl;

        if (x < 0) break;

        if ((tickCounter % minorTickDivisions_) == 0) {
            QLineF line(x, 0.0, x, majorTickHeight_);
            painter.drawLine(line);
            QString label = formatTickTag(tag);
            double offset = fm.boundingRect(label).width() / 2;
            if (x - offset < 0.0) { offset = 0.0; }

            LOGDEBUG << "tag: " << tag << " offset: " << offset << std::endl;

            line.translate(-offset, tagOffset - fm.descent() - 1);
            painter.drawText(line.p1(), label);
            tag -= tagIncrement;
        } else {
            QLineF line(x, 0.0, x, minorTickHeight_);
            painter.drawLine(line);
        }

        ++tickCounter;
    }

    tickCounter = 1;
    tag = tagIncrement;

    while (1) {
        double x = shift + tickCounter * minorIncrement_;
        if (x >= span) break;

        if ((tickCounter % minorTickDivisions_) == 0) {
            QLineF line(x, 0.0, x, majorTickHeight_);
            painter.drawLine(line);
            QString label = formatTickTag(tag);
            double offset = fm.boundingRect(label).width() / 2;
            if (x + offset > span) { offset += offset; }

            line.translate(-offset, tagOffset - fm.descent() - 1);
            painter.drawText(line.p1(), label);
            tag += tagIncrement;
        } else {
            QLineF line(x, 0.0, x, minorTickHeight_);
            painter.drawLine(line);
        }

        ++tickCounter;
    }
}

void
ScaleWidget::drawGridLines(QPainter& painter)
{
    QPaintDevice* device = painter.device();
    recalculateTickIntervals(device->width(), device->height());
    painter.setRenderHint(QPainter::Antialiasing, false);
    if (orientation_ == Qt::Horizontal) {
        drawVerticalGridLines(painter, device->width(), device->height());
    } else {
        drawHorizontalGridLines(painter, device->width(), device->height());
    }
}

void
ScaleWidget::drawVerticalGridLines(QPainter& painter, int width, int height) const
{
    for (int major = 0; major < majorTickDivisions_; ++major) {
        double x = major * majorIncrement_;
        if (major) {
            painter.setPen(majorGridPen_);
            QLineF line(x, 0.0, x, height);
            painter.drawLine(line);
        }

        painter.setPen(minorGridPen_);
        for (int minor = 1; minor < minorTickDivisions_; ++minor) {
            x += minorIncrement_;
            QLineF line(x, 0.0, x, height);
            painter.drawLine(line);
        }
    }
}

void
ScaleWidget::drawHorizontalGridLines(QPainter& painter, int width, int height) const
{
    static Logger::ProcLog log("drawHorizontalGridLines", Log());

    for (int major = 0; major < majorTickDivisions_; ++major) {
        double y = height - major * majorIncrement_;
        LOGERROR << "major: " << major << " y: " << y << std::endl;
        if (major) {
            painter.setPen(majorGridPen_);
            QLineF line(0.0, y, width, y);
            painter.drawLine(line);
        }

        painter.setPen(minorGridPen_);
        for (int minor = 1; minor < minorTickDivisions_; ++minor) {
            y -= minorIncrement_;
            QLineF line(0.0, y, width, y);
            painter.drawLine(line);
        }
    }
}

void
ScaleWidget::setMajorTickHeight(int height)
{
    majorTickHeight_ = height;
    update();
}

void
ScaleWidget::setMinorTickHeight(int height)
{
    minorTickHeight_ = height;
    update();
}

void
ScaleWidget::setMajorTickDivisions(int divisions)
{
    majorTickDivisionsMax_ = divisions;
    lastSizeCalculated_ = QSize(-1, -1);
    update();
}

void
ScaleWidget::setMinorTickDivisions(int divisions)
{
    minorTickDivisionsMax_ = divisions;
    lastSizeCalculated_ = QSize(-1, -1);
    update();
}

void
ScaleWidget::setMajorAndMinorTickDivisions(int major, int minor)
{
    majorTickDivisionsMax_ = major;
    minorTickDivisionsMax_ = minor;
    lastSizeCalculated_ = QSize(-1, -1);
    update();
}

void
ScaleWidget::setStartAndEnd(double start, double end)
{
    start_ = start;
    range_ = end - start_;
    update();
}

void
ScaleWidget::setStartAndRange(double start, double range)
{
    start_ = start;
    range_ = range;
    update();
}

void
ScaleWidget::setStart(double start)
{
    start_ = start;
    update();
}

void
ScaleWidget::setEnd(double end)
{
    range_ = end - start_;
    update();
}

void
ScaleWidget::setMidPoint(double zero)
{
    start_ = zero - range_ / 2.0;
    update();
}

void
ScaleWidget::setRange(double range)
{
    range_ = range;
    update();
}

void
ScaleWidget::setOrientation(Qt::Orientation orientation)
{
    orientation_ = orientation;
    if (orientation_ == Qt::Horizontal) {
        setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    } else {
        setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    }
    update();
}

void
ScaleWidget::drawCursorPosition(QPainter& painter, int position)
{
    painter.setPen(cursorIndicatorPen_);
    painter.setBrush(cursorIndicatorPen_.color());
    QPolygonF indicator;
    if (orientation_ == Qt::Horizontal) {
        indicator << QPointF(position, 0) << QPointF(position - 4, majorTickHeight_)
                  << QPointF(position + 4, majorTickHeight_) << QPointF(position, 0);
    } else {
        indicator << QPointF(width() - 1, position) << QPointF(width() - 1 - majorTickHeight_, position - 4)
                  << QPointF(width() - 1 - majorTickHeight_, position + 4) << QPointF(width() - 1, position);
    }

    painter.drawConvexPolygon(indicator);
}

void
ScaleWidget::dirtyCursorPosition(int position)
{
    if (position != -1) {
        if (orientation_ == Qt::Horizontal) {
            update(position - 5, 0, 11, height() - 1);
        } else {
            update(0, position - 5, width() - 1, 11);
        }
    }
}

void
ScaleWidget::setCursorPosition(double value)
{
    int position = int(::rint((value - start_) / range_ * (getSpan() - 1)));
    if (orientation_ == Qt::Vertical) position = height() - 1 - position;
    if (position != cursorPosition_) {
        dirtyCursorPosition(cursorPosition_);
        cursorPosition_ = position;
        dirtyCursorPosition(cursorPosition_);
    }
}

void
ScaleWidget::setNormalizedCursorPosition(double value)
{
    int position = int(::rint(value * (getSpan() - 1)));
    if (orientation_ == Qt::Vertical) position = height() - 1 - position;
    if (position != cursorPosition_) {
        dirtyCursorPosition(cursorPosition_);
        cursorPosition_ = position;
        dirtyCursorPosition(cursorPosition_);
    }
}

QString
DegreesScaleWidget::formatTickTag(double value)
{
    return QString("%1%2").arg(Utils::radiansToDegrees(Utils::normalizeRadians(value))).arg(GUI::DegreeSymbol());
}
