#ifndef SIDECAR_GUI_SQUAREBUTTON_H // -*- C++ -*-
#define SIDECAR_GUI_SQUAREBUTTON_H

#include "QtDesigner/QDesignerExportWidget"
#include "QtGui/QPushButton"

#include "GUI/LED.h"

namespace SideCar {
namespace GUI {

/** Generic square UI widget button that supports on/off color states and a 'pending' state for transitioning
    between the two.
*/
class QDESIGNER_WIDGET_EXPORT SquareButton : public QPushButton {
    using Super = QPushButton;

    Q_OBJECT;
    Q_PROPERTY(int length READ getLength WRITE setLength);
    Q_PROPERTY(SideCar::GUI::LED::Color onColor READ getOnColor WRITE setOnColor);
    Q_PROPERTY(SideCar::GUI::LED::Color offColor READ getOffColor WRITE setOffColor);
    Q_PROPERTY(SideCar::GUI::LED::Color pendingColor READ getPendingColor WRITE setPendingColor);
    Q_PROPERTY(bool showStatus READ getShowStatus WRITE setShowStatus);
    Q_PROPERTY(bool value READ getValue WRITE setValue);
    Q_PROPERTY(bool pending READ getPending WRITE setPending);
    Q_PROPERTY(QPoint statusPosition READ getStatusPosition WRITE setStatusPosition);

public:
    SquareButton(QWidget* parent = 0);

    int getLength() const { return length_; }
    LED::Color getOnColor() const { return onColor_; }
    LED::Color getOffColor() const { return offColor_; }
    LED::Color getPendingColor() const { return pendingColor_; }
    bool getShowStatus() const { return showStatus_; }
    bool getValue() const { return Super::isChecked(); }
    bool getPending() const { return pending_; }
    QPoint getStatusPosition() const { return led_->pos(); }

public slots:

    void setLength(int value);
    void setOnColor(SideCar::GUI::LED::Color value);
    void setOffColor(SideCar::GUI::LED::Color value);
    void setPendingColor(SideCar::GUI::LED::Color value);
    void setShowStatus(bool value);
    void setValue(bool value);
    void setPending(bool value);
    void setStatusPosition(const QPoint& value);

private:
    void checkStateSet();
    void nextCheckState();
    void updateStatus();

    SideCar::GUI::LED* led_;
    int length_;
    LED::Color onColor_;
    LED::Color offColor_;
    LED::Color pendingColor_;
    bool showStatus_;
    bool pending_;
};

} // namespace GUI
} // namespace SideCar

#endif
