#include "QtGui/QColorDialog"

#include "ColorButtonWidget.h"

using namespace SideCar::GUI;

ColorButtonWidget::ColorButtonWidget(QWidget* parent)
    : Super(parent)
{
    setAutoFillBackground(true);
    setAutoDefault(false);
    setFlat(true);
    setMinimumSize(32, 32);
    setMaximumSize(32, 32);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(this, SIGNAL(clicked()), this, SLOT(editColor()));
}

QColor
ColorButtonWidget::getColor() const
{
    ensurePolished();
    return palette().color(QPalette::Window);
}

void
ColorButtonWidget::setColor(const QColor& color)
{
    if (getColor() == color) return;
    QPalette p(palette());
    p.setColor(QPalette::Window, color);
    p.setColor(QPalette::Button, color);
    setPalette(p);
    emit colorChanged(color);
}

void
ColorButtonWidget::editColor()
{
    QColor color = QColorDialog::getColor(getColor(), 0);
    if (color.isValid())
	setColor(color);
}
