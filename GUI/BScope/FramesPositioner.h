#ifndef SIDECAR_GUI_BSCOPE_FRAMESPOSITIONER_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_FRAMESPOSITIONER_H

#include "ui_FramesPositioner.h"

namespace SideCar {
namespace GUI {
namespace BScope {

class FramesPositioner : public QWidget, private Ui::FramesPositioner {
    Q_OBJECT
    using Super = QWidget;

public:
    FramesPositioner(QWidget* parent = 0);

    void updateInfo();

    int getPosition() const { return position_->value(); }

    int getMaximum() const { return position_->maximum(); }

    void setPosition(int position);

    void setMaximum(int maximum);

signals:

    void positionChanged(int position);

private slots:

    void on_position__valueChanged(int value);
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
