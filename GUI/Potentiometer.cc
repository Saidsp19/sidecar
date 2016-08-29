#include <cmath>

#include "QtGui/QPainter"
#include "QtGui/QWheelEvent"

#include "Potentiometer.h"

using namespace SideCar::GUI;

Potentiometer::Potentiometer(QWidget* parent)
    : QWidget(parent), value_(0.0), minimum_(0.0), maximum_(100.0),
      step_(1.0), tickCount_(25), precision_(0),
      startColor_(Qt::green), endColor_(Qt::red), showScrews_(true),
      showSweep_(true), showValue_(true)
{
    setFocusPolicy(Qt::StrongFocus);
}

void
Potentiometer::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 200.0, side / 200.0);

    paintBorder(painter);

    if (showSweep_)
	paintSweep(painter);

    paintKnob(painter);
    paintTicks(painter);

    if (showValue_)
	paintValue(painter);
}

void
Potentiometer::paintBorder(QPainter& painter)
{
    // Draw the background gradiant.
    //
    painter.setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    {
	QLinearGradient grad(-100, 0, 100, -20);
	grad.setColorAt(0, Qt::darkGray);
	grad.setColorAt(1, Qt::white);
	grad.setSpread(QGradient::ReflectSpread);
	painter.setBrush(grad);
    }

    QRectF border(-100, -100, 200, 200);
    painter.drawRect(border);

    if (showScrews_) {
	painter.setPen(QPen(Qt::black, 0, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
	painter.drawEllipse(-90, -90, 10, 10);
	painter.drawLine(-87, -87, -83, -83);
	painter.drawLine(-83, -87, -87, -83);

	painter.drawEllipse(80, -90, 10, 10);
	painter.drawLine(83, -87, 87, -83);
	painter.drawLine(87, -87, 83, -83);

	painter.drawEllipse(-90, 80, 10, 10);
	painter.drawLine(-87, 83, -83, 87);
	painter.drawLine(-83, 83, -87, 87);

	painter.drawEllipse(80, 80, 10, 10);
	painter.drawLine(83, 83, 87, 87);
	painter.drawLine(87, 83, 83, 87);
    }

    // Draw sweep arc background
    //
    {
	QLinearGradient grad(-80, 50, 110, 60);
	grad.setColorAt(0, Qt::white);
	grad.setColorAt(1, Qt::darkGray);
	grad.setSpread(QGradient::PadSpread);
	painter.setBrush(grad);
    }

    painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    QRectF rectangle(-70, -70, 140, 140);
    painter.drawPie(rectangle, 225 * 16, -270 * 16);
}

void
Potentiometer::paintKnob(QPainter& painter)
{
    // Draw the knob in a brushed metal way
    //
    painter.setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    QLinearGradient grad(-80, 40, -10, -10);
    if (hasFocus())
	grad.setColorAt(0, Qt::blue);
    else
	grad.setColorAt(0, Qt::darkGray);
    grad.setColorAt(1, Qt::white);
    grad.setSpread(QGradient::ReflectSpread);
    painter.setBrush(grad);
    painter.drawEllipse(QRectF(-50, -50, 100, 100));

    // Draw inset dotted circle
    //
    painter.setPen(QPen(Qt::black, 1, Qt::DotLine, Qt::RoundCap,
                        Qt::RoundJoin));
    painter.drawEllipse(QRectF(-45, -45, 90, 90));

    int pos = (value_ - minimum_) / (maximum_ - minimum_) * 270 - 225;

    // Draw an indicator line at the correct setting.
    //
    painter.save();
    painter.setPen(QPen(Qt::black, 5, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    painter.rotate(pos);
    painter.drawLine(0, 0, 40, 0);

    // Draw red dot at the current value
    //
    painter.setPen(QPen(Qt::red, 5, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    painter.drawPoint(45, 0);
    painter.restore();
}

void
Potentiometer::paintTicks(QPainter& painter)
{
    float start = M_PI + M_PI / 4;
    float range = M_PI + M_PI / 2;
    float delta = range / tickCount_;

    painter.setPen(QPen(Qt::black, 3, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    for (int i = 0; i <= tickCount_; ++i) {
        int x =  75 * ::cosf(start - delta * i);
        int y = -75 * ::sinf(start - delta * i);
        painter.drawPoint(x, y);
    }
}

void
Potentiometer::paintSweep(QPainter& painter)
{
    QLinearGradient grad(-20, 0, 40, 40);
    grad.setColorAt(0, startColor_);
    grad.setColorAt(1, endColor_);
    grad.setSpread(QGradient::PadSpread);
    painter.setBrush(grad);
    painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    int span = (value_ - minimum_) * 270 * 16 / (maximum_ - minimum_);
    painter.drawPie(QRectF(-70, -70, 140, 140), 225 * 16, -span);
}

void
Potentiometer::paintValue(QPainter& painter)
{
    painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    painter.setBrush(Qt::black);

    QRect valueRect(-30, 55, 60, 35);
    painter.drawRect(valueRect);

    QString valueText = QString("%1").arg(value_, 0, 'f', precision_);
    painter.setPen(QPen(Qt::green));
    painter.setFont(QFont("Arial", 24));
    painter.drawText(valueRect, Qt::AlignCenter, valueText);
}

void
Potentiometer::setMininum(double value)
{
    minimum_ = value;
    if (value >= maximum_)
	maximum_ = value + 1;
    if (value_ > minimum_)
	value = value_;
    setValue(value);
}

void
Potentiometer::setMaximum(double value)
{
    maximum_ = value;
    if (value <= minimum_)
	minimum_ = value - 1;
    if (value_ < maximum_)
	value = value_;
    setValue(value);
}

void
Potentiometer::setValue(double value)
{
    if (value < minimum_)
	value = minimum_;
    if (value > maximum_)
	value = maximum_;
    if (value != value_) {
	value_ = value;
	emit valueChanged(value);
	update();
    }
}

void
Potentiometer::setTickCount(int value)
{
    tickCount_ = value;
    update();
}

void
Potentiometer::setStep(double value)
{
    step_ = value;
}

void
Potentiometer::setPrecision(int value)
{
    precision_ = value;
    update();
}

void
Potentiometer::setStartColor(QColor value)
{
    startColor_ = value;
    update();
}

void
Potentiometer::setEndColor(QColor value)
{
    endColor_ = value;
    update();
}

void
Potentiometer::setShowScrews(bool value)
{
    showScrews_ = value;
    update();
}

void
Potentiometer::setShowSweep(bool value)
{
    showSweep_ = value;
    update();
}

void
Potentiometer::setShowValue(bool value)
{
    showValue_ = value;
    update();
}

void
Potentiometer::wheelEvent(QWheelEvent* event)
{
    double numDegrees = event->delta() / 8;
    double numSteps = numDegrees / 15;
    setValue(value_ + numSteps * step_);
}

void
Potentiometer::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Down:
    case Qt::Key_Comma:
    case Qt::Key_Minus:
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
	setValue(value_ - step_);
	break;

    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Period:
    case Qt::Key_Plus:
    case Qt::Key_Equal:
    case Qt::Key_Space:
	setValue(value_ + step_);
	break;
	
    default:
	QWidget::keyPressEvent(event);
	break;
    }
}

QSize
Potentiometer::minimumSizeHint() const
{
    return QSize(80, 80);
}

QSize
Potentiometer::sizeHint() const
{
    return QSize(200, 200);
}
