#include "CursorWidget.h"

using namespace SideCar::GUI::Spectrum;

CursorWidget::CursorWidget(QWidget* parent)
    : Super(parent)
{
    showCursorPosition("X: 0 Hz Y: 0 db");
}

void
CursorWidget::showCursorPosition(const QString& value)
{
    setValue(value);
    updateGeometry();
}
