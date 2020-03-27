#include <algorithm>
#include <cmath>
#include <functional>

#include "QtGui/QGuiApplication"
#include "QtGui/QPainter"

#include "ScaleWidget.h"

using namespace SideCar::GUI::AScope;

ScaleWidget::ScaleWidget(QWidget* parent, Qt::Orientation orientation) :
    QWidget(parent), lastSizeCalculated_(), majorTickHeight_(10), minorTickHeight_(6), majorTickDivisionsMax_(8),
    minorTickDivisionsMax_(4), start_(0.0), range_(4.0), orientation_(orientation),
    majorGridPen_(QColor(64, 64, 64), 0.0), minorGridPen_(QColor(16, 16, 16), 0.0),
    cursorIndicatorPen_(QColor(255, 0, 0), 0), cursorPosition_(-1)
{
    QFont labelFont(font());
    labelFont.setPointSize(10);
    setFont(labelFont);
    setBackgroundRole(QPalette::Base);
    setOrientation(orientation);
    setCursor(Qt::CrossCursor);
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
    double span = orientation_ == Qt::Horizontal ? width : height;
    majorIncrement_ = std::max(80.0, span / majorTickDivisionsMax_);
    majorTickDivisions_ = std::max(1, int(::floor(span / majorIncrement_)));
    majorIncrement_ = span / majorTickDivisions_;
    minorIncrement_ = std::max(40.0, majorIncrement_ / minorTickDivisionsMax_);
    minorTickDivisions_ = std::max(1, int(::floor(majorIncrement_ / minorIncrement_)));
    minorIncrement_ = majorIncrement_ / minorTickDivisions_;
}

void
ScaleWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    render(painter, width(), height());
    if (cursorPosition_ != -1) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        drawCursorPosition(painter, cursorPosition_);
    }
}

void
ScaleWidget::render(QPainter& painter, int width, int height)
{
    recalculateTickIntervals(width, height);

    // For horizontal and vertical lines, disable anti-aliasing.
    //
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    if (orientation_ == Qt::Horizontal) {
        drawTicks(painter, width - 1, 0.0, 1, height);
    } else {
        painter.save();
        painter.rotate(-90.0);
        painter.translate(-height, 0.0);
        drawTicks(painter, height - 1, width - 1, -1, -majorTickHeight_);
        painter.restore();
    }
}

void
ScaleWidget::drawTicks(QPainter& painter, int width, double y, int sign, int tagOffset)
{
    auto palette = QGuiApplication::palette();

    painter.setFont(font());
    painter.setPen(palette.dark().color());

    QFontMetrics fm(fontMetrics());
    double tag = start_;
    double tagIncrement = range_ / majorTickDivisions_;

    for (int major = 0; major <= majorTickDivisions_; ++major) {
        double x = major * majorIncrement_;
        QLineF line(x, y, x, y + sign * majorTickHeight_);
        painter.drawLine(line);

        QString label = QString("%1").arg(tag, 0, 'f', 1);
        double offset = 0.0;
        if (major == majorTickDivisions_) {
            offset = -fm.boundingRect(label).width();
        } else if (major != 0) {
            offset = -fm.boundingRect(label).width() / 2;
        }

        line.translate(offset, tagOffset - fm.descent() - 1);
        painter.drawText(line.p1(), label);
        tag += tagIncrement;

        if (major < majorTickDivisions_) {
            for (int minor = 1; minor < minorTickDivisions_; ++minor) {
                x += minorIncrement_;
                line = QLineF(x, y, x, y + sign * minorTickHeight_);
                painter.drawLine(line);
            }
        }
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
    for (int major = 0; major < majorTickDivisions_; ++major) {
        double y = height - major * majorIncrement_;
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
    update();
}

void
ScaleWidget::setMinorTickDivisions(int divisions)
{
    minorTickDivisionsMax_ = divisions;
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
    if (end <= start_) return;
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
    if (range <= 0.0 || range == range_) return;
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
ScaleWidget::setCursorPosition(int position)
{
    dirtyCursorPosition(cursorPosition_);
    cursorPosition_ = position;
    dirtyCursorPosition(position);
}
