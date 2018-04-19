#ifndef SIDECAR_GUI_LED_H // -*- C++ -*-
#define SIDECAR_GUI_LED_H

#include "QtUiPlugin/QDesignerExportWidget"
#include "QtSvg/QSvgWidget"

namespace SideCar {
namespace GUI {

class QDESIGNER_WIDGET_EXPORT LED : public QSvgWidget {
    using Super = QSvgWidget;

    Q_OBJECT;
    Q_ENUMS(Color);
    Q_PROPERTY(Color color READ getColor WRITE setColor);
    Q_PROPERTY(int diameter READ getDiameter WRITE setDiameter);

public:
    enum Color { kGrey, kRed, kGreen, kBlue, kYellow, kOrange, kTurquoise };

    LED(QWidget* parent = 0);

    Color getColor() const { return color_; }

    int getDiameter() const { return diameter_; }

public slots:

    void setColor(Color value);
    void setDiameter(int value);
    void cycleColors();
    void setGreyColor() { setColor(kGrey); }
    void setRedColor() { setColor(kRed); }
    void setGreenColor() { setColor(kGreen); }
    void setBlueColor() { setColor(kBlue); }
    void setYellowColor() { setColor(kYellow); }
    void setOrangeColor() { setColor(kOrange); }
    void setTurquoiseColor() { setColor(kTurquoise); }

private:
    Color color_;
    int diameter_;
};

} // namespace GUI
} // namespace SideCar

#endif
