#include "SquareButton.h"

using namespace SideCar::GUI;

SquareButton::SquareButton(QWidget* parent) :
    Super(parent), led_(new SideCar::GUI::LED(this)), length_(0), onColor_(LED::kRed), offColor_(LED::kGrey),
    pendingColor_(LED::kYellow), showStatus_(true), pending_(false)
{
    setCheckable(true);
    setAutoExclusive(true);

    led_->setDiameter(12);
    led_->setColor(offColor_);
    setLength(75);

    setStyleSheet("QPushButton { "
                  "  border: 3px solid #000080; "
                  "  border-radius: 14px; "
                  "  background-color: #0000CC; "
                  "  min-width: 80px; "
                  "  color: #FFFFFF; "
                  "} "
                  "QPushButton:pressed { "
                  "  border: 3px solid #8080FF; "
                  "} "
                  " QPushButton:checked { "
                  "  border: 3px solid #8080FF; "
                  "  color: #FFFF00; "
                  " }");
}

void
SquareButton::setLength(int value)
{
    if (value != length_) {
        length_ = value;
        QSize size(value, value);
        setMinimumSize(size);
        setMaximumSize(size);
        led_->move(width() - 10 - 12, height() - 10 - 12);
    }
}

void
SquareButton::setOnColor(LED::Color value)
{
    if (value != onColor_) {
        onColor_ = value;
        updateStatus();
    }
}

void
SquareButton::setOffColor(LED::Color value)
{
    if (value != offColor_) {
        offColor_ = value;
        updateStatus();
    }
}

void
SquareButton::setPendingColor(LED::Color value)
{
    if (value != pendingColor_) {
        pendingColor_ = value;
        updateStatus();
    }
}

void
SquareButton::setShowStatus(bool value)
{
    if (value != showStatus_) {
        showStatus_ = value;
        led_->setVisible(value);
    }
}

void
SquareButton::setValue(bool state)
{
    if (pending_) {
        if (state == isChecked()) { pending_ = false; }
    } else {
        pending_ = true;
        setChecked(state);
    }

    updateStatus();
}

void
SquareButton::setPending(bool value)
{
    if (value != pending_) {
        pending_ = value;
        updateStatus();
    }
}

void
SquareButton::setStatusPosition(const QPoint& value)
{
    led_->move(value);
}

void
SquareButton::checkStateSet()
{
    pending_ = false;
    Super::checkStateSet();
    updateStatus();
}

void
SquareButton::nextCheckState()
{
    pending_ = true;
    Super::nextCheckState();
    updateStatus();
}

void
SquareButton::updateStatus()
{
    if (pending_) {
        led_->setColor(pendingColor_);
    } else if (isChecked()) {
        led_->setColor(onColor_);
    } else {
        led_->setColor(offColor_);
    }
}
