#ifndef SIDECAR_GUI_ESSCOPE_CONTROLSWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_CONTROLSWIDGET_H

#include "QtCore/QList"
#include "QtGui/QWidget"

class QBoxLayout;
class QSlider;
class QToolBar;

namespace SideCar {
namespace GUI {

class QSliderSetting;

class ControlsWidget : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    ControlsWidget(QToolBar* parent);

    Qt::Orientation getOrientation() const;

    void addControl(const QString& tage, QSliderSetting* setting);

public slots:

    void changeOrientation(Qt::Orientation orientation);

private:
    void updateOrientation(Qt::Orientation orientation, QSlider* control);

    QList<QSlider*> controls_;
    QBoxLayout* layout_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
