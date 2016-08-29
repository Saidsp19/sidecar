#ifndef SIDECAR_GUI_PPIDISPLAY_CONTROLWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_CONTROLWIDGET_H

#include "QtGui/QWidget"

class QBoxLayout;
class QLabel;
class QSlider;
class QToolBar;

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

class ControlWidget : public QWidget
{
    Q_OBJECT
    using Super = QWidget;
public:

    ControlWidget(const QString& tag, QToolBar* parent);

    QSlider* getControl() const { return control_; }

public slots:

    void changeOrientation(Qt::Orientation orientation);

private:

    void updateOrientation(Qt::Orientation orientation);

    QSlider* control_;
    QBoxLayout* layout_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

#endif
