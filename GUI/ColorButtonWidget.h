#ifndef SIDECAR_GUI_COLORBUTTONWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_COLORBUTTONWIDGET_H

#include "QtWidgets/QPushButton"

namespace SideCar {
namespace GUI {

/** Derivation of QPushButton that shows a color setting, and provides a way to edit the color value when
    pressed.
*/
class ColorButtonWidget : public QPushButton {
    Q_OBJECT
    using Super = QPushButton;

public:
    /** Constructor with widget to manage.

        \param mgr PresetManager object that records this setting

        \param widget the QPushButton widget to use

        \param global if true, this is a global setting, not associated with a
        specific Preset.
    */
    ColorButtonWidget(QWidget* parent = 0);

    QColor getColor() const;

signals:

    void colorChanged(const QColor& color);

public slots:

    void setColor(const QColor& color);

    /** Event handler called when the button given in the constructor is pushed.
     */
    void editColor();
};

} // end namespace GUI
} // end namespace SideCar

#endif
