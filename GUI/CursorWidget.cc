#include "CursorWidget.h"

#include "ui_CursorWidget.h"

using namespace SideCar::GUI;

CursorWidget::CursorWidget(QWidget* parent) : Super(parent), gui_(new Ui::CursorWidget)
{
    gui_->setupUi(this);
    gui_->value_->setTextFormat(Qt::PlainText);
}

QString
CursorWidget::getValue() const
{
    return gui_->value_->text();
}

void
CursorWidget::setValue(const QString& value)
{
    gui_->value_->setText(value);
}
