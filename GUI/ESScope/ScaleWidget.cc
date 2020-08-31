#include <algorithm>
#include <cmath>
#include <functional>

#include "QtGui/QPainter"

#include "GUI/LogUtils.h"
#include "Utils/Utils.h"

#include "ScaleWidget.h"

using namespace SideCar::GUI::ESScope;

Logger::Log&
ScaleWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("esscope.ScaleWidget");
    return log_;
}

ScaleWidget::ScaleWidget(QWidget* parent, Qt::Orientation orientation) :
    QWidget(parent), label_(""), majorTickHeight_(10), minorTickHeight_(3), majorTickDivisionsMax_(8),
    minorTickDivisionsMax_(4), start_(0.0), range_(4.0), orientation_(orientation),
    majorGridPen_(QColor(64, 64, 64), 0.0), minorGridPen_(QColor(32, 32, 32), 0.0),
    cursorIndicatorPen_(QColor(255, 0, 0, 128), 0), cursorPosition_(-1), autoDivide_(false)
{
    QFont labelFont(font());
    labelFont.setPointSize(10);
    setFont(labelFont);
    setBackgroundRole(QPalette::Base);
    setOrientation(orientation);
}

void
ScaleWidget::setLabel(const QString& label)
{
    label_ = label;
    makeTicks();
    update();
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
    recalculateTickIntervals();
}

void
ScaleWidget::resizeEvent(QResizeEvent* event)
{
    static Logger::ProcLog log("resizeEvent", Log());
    LOGINFO << "width: " << width() << " height: " << height() << std::endl;

    Super::resizeEvent(event);

    if (orientation_ == Qt::Vertical) {
        transform_.reset();
        transform_.translate(width(), 0);
        transform_.rotate(90.0);
    }

    recalculateTickIntervals();
}

QSize
ScaleWidget::sizeHint() const
{
    if (orientation_ == Qt::Horizontal)
        return QSize(width(), majorTickHeight_ + fontMetrics().height() * 2);
    else
        return QSize(majorTickHeight_ + fontMetrics().height() * 2, height());
}

#if 0
QSize
ScaleWidget::minimumSizeHint() const
{
  return sizeHint();
}
#endif

void
ScaleWidget::setAutoDivide(bool autoDivide)
{
    autoDivide_ = autoDivide;
    recalculateTickIntervals();
}

double
ScaleWidget::getTickSpan() const
{
    return getSpan() - 1;
}

double
ScaleWidget::getTagOffset() const
{
    return (orientation_ == Qt::Horizontal ? height() : width()) - fontMetrics().height() - 1;
}

void
ScaleWidget::recalculateTickIntervals()
{
    static Logger::ProcLog log("recalculateTickIntervals", Log());
    LOGINFO << std::endl;

    double span = getTickSpan();

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

    makeTicks();
}

void
ScaleWidget::makeTicks()
{
    static Logger::ProcLog log("makeTicks", Log());
    LOGINFO << std::endl;

    majorTicks_.clear();
    minorTicks_.clear();
    labelStarts_.clear();
    labelTexts_.clear();

    QFontMetrics fm(fontMetrics());

    double span = getTickSpan();
    double tagOffset = getTagOffset();

    double tag = start_;
    int tickCounter = 0;
    double tagIncrement = range_ / majorTickDivisions_;

    LOGDEBUG << "tagIncrement: " << tagIncrement << std::endl;

    while (1) {
        double x = tickCounter * minorIncrement_;
        LOGDEBUG << "x: " << x << " tickCounter: " << tickCounter << std::endl;

        if ((tickCounter % minorTickDivisions_) == 0) {
            QLineF line(x, 0.0, x, majorTickHeight_);
            if (orientation_ == Qt::Vertical && x >= span) line.translate(span - x - 1, 0);

            majorTicks_.push_back(line);

            if (::fabs(tag) < 1E-6) tag = 0.0;

            QString label = formatTickTag(tag);
            labelTexts_.push_back(label);

            double offset = fm.boundingRect(label).width() / 2.0;
            if (x - offset < 0.0) {
                offset = 0.0;
            } else if (x + offset >= span) {
                offset += offset;
            }

            line.translate(-offset, tagOffset - fm.descent() - 1);
            labelStarts_.push_back(line.p1());

            tag += tagIncrement;

            if (x >= span) break;
        } else {
            minorTicks_.push_back(QLineF(x, 0.0, x, minorTickHeight_));
        }

        ++tickCounter;
    }

    if (!label_.isEmpty()) {
        double offset = fm.boundingRect(label_).width() / 2.0;
        QPointF pt(getSpan() / 2.0, 0.0);
        pt += QPointF(-offset, tagOffset + fm.height() - fm.descent() - 1);
        labelPosition_ = pt.toPoint();
    }

    update();
}

void
ScaleWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    // For tick marks, disable antialiasing.
    //
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    painter.setFont(font());
    painter.setPen(Qt::black);

    if (orientation_ == Qt::Vertical) painter.setWorldTransform(transform_);

    painter.drawLines(majorTicks_);
    painter.drawLines(minorTicks_);

    for (int index = 0; index < labelStarts_.size(); ++index) {
        painter.drawText(labelStarts_[index], labelTexts_[index]);
    }

    if (!label_.isEmpty()) painter.drawText(labelPosition_, label_);

    if (cursorPosition_ != -1) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(cursorIndicatorPen_);
        painter.setBrush(cursorIndicatorPen_.color());
        QPolygonF indicator;
        indicator << QPointF(cursorPosition_, 0) << QPointF(cursorPosition_ - 4, majorTickHeight_)
                  << QPointF(cursorPosition_ + 4, majorTickHeight_) << QPointF(cursorPosition_, 0);
        painter.drawConvexPolygon(indicator);
    }
}

void
ScaleWidget::drawGridLines(QPainter& painter)
{
    QPaintDevice* device = painter.device();
    painter.setRenderHint(QPainter::Antialiasing, false);
    if (orientation_ == Qt::Horizontal) {
        drawGridLines(painter, device->height());
    } else {
        painter.save();
        painter.translate(device->width(), 0);
        painter.rotate(90.0);
        drawGridLines(painter, device->width());
        painter.restore();
    }
}

void
ScaleWidget::drawGridLines(QPainter& painter, int height) const
{
    QVector<QLineF> lines;
    painter.setPen(majorGridPen_);
    for (int index = 0; index < majorTicks_.size(); ++index) {
        qreal x = majorTicks_[index].x1();
        lines.push_back(QLineF(x, 0.0, x, height));
    }

    painter.drawLines(lines);
    lines.clear();

    painter.setPen(minorGridPen_);
    for (int index = 0; index < minorTicks_.size(); ++index) {
        qreal x = minorTicks_[index].x1();
        lines.push_back(QLineF(x, 0.0, x, height));
    }

    painter.drawLines(lines);
}

void
ScaleWidget::setMajorTickHeight(int height)
{
    majorTickHeight_ = height;
    makeTicks();
}

void
ScaleWidget::setMinorTickHeight(int height)
{
    minorTickHeight_ = height;
    makeTicks();
}

void
ScaleWidget::setMajorTickDivisions(int divisions)
{
    majorTickDivisionsMax_ = divisions;
    recalculateTickIntervals();
}

void
ScaleWidget::setMinorTickDivisions(int divisions)
{
    minorTickDivisionsMax_ = divisions;
    recalculateTickIntervals();
}

void
ScaleWidget::setStart(double start)
{
    start_ = start;
    makeTicks();
    update();
}

void
ScaleWidget::setEnd(double end)
{
    range_ = end - start_;
    makeTicks();
    update();
}

void
ScaleWidget::setStartAndEnd(double start, double end)
{
    start_ = start;
    setEnd(end);
}

void
ScaleWidget::setMidPoint(double zero)
{
    start_ = zero - range_ / 2.0;
    makeTicks();
    update();
}

void
ScaleWidget::setRange(double range)
{
    range_ = range;
    makeTicks();
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

    recalculateTickIntervals();
}

void
ScaleWidget::setCursorPosition(int position)
{
    cursorPosition_ = position;
    update();
}

void
ScaleWidget::setCursorValue(double value)
{
    cursorPosition_ = int(::rint((value - start_) * double(getSpan()) / range_));
    update();
}

std::vector<float>
ScaleWidget::getMajorTickPositions() const
{
    std::vector<float> positions;
    for (int index = 0; index < majorTicks_.size(); ++index) {
        positions.push_back(majorTicks_[index].x1() / getTickSpan());
    }

    return positions;
}

std::vector<float>
ScaleWidget::getMinorTickPositions() const
{
    std::vector<float> positions;
    for (int index = 0; index < minorTicks_.size(); ++index) {
        positions.push_back(minorTicks_[index].x1() / getTickSpan());
    }

    return positions;
}
