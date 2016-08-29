#include <algorithm>
#include <cmath>

#include "QtGui/QColor"
#include "QtGui/QPainter"
#include "QtGui/QPolygon"
#include "QtGui/QRadialGradient"

#include "QLed.h"

QLed::QLed(QWidget* parent)
    : QWidget(parent), value_(false), pending_(false), color_(Qt::red)
{
    ;
}

void
QLed::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    painter.drawEllipse(1, 1, width() - 2, height() - 2);

    if (value_) {
	QRadialGradient radialGrad(QPointF(0.32 * width(), 0.32 * height()),
                                   0.68 * std::min(width(), height()));
	radialGrad.setColorAt(0, color_.lighter(190));
	radialGrad.setColorAt(1, color_);
	painter.setPen(Qt::NoPen);
	painter.setBrush(radialGrad);
	painter.drawEllipse(1, 1, width() - 2, height() - 2);
    }

    if (pending_) {

	// Dim the LED with a grey overlay.
	//
	painter.setBrush(QColor(128, 128, 128, 128));
	painter.drawEllipse(1, 1, width() - 2, height() - 2);
    }
}

void
QLed::setColor(const QColor& value)
{
    color_ = value;
    update();
}

void
QLed::setValue(bool value)
{
    value_ = value;
    pending_ = false;
    update();
}

void
QLed::setPending(bool value)
{
    pending_ = value;
    update();
}

void
QLed::toggleValue()
{ 
    setValue(! getValue());
    update();
}
