#ifndef SIDECAR_GUI_BSCOPE_PLAYERPOSITIONER_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_PLAYERPOSITIONER_H

#include "ui_PlayerPositioner.h"

namespace SideCar {
namespace GUI {
namespace BScope {

class PlayerSettings;

class PlayerPositioner : public QWidget, private Ui::PlayerPositioner {
    Q_OBJECT
    using Super = QWidget;

public:
    PlayerPositioner(PlayerSettings* settings, QWidget* parent = 0);

    void updateButtons();

    void updateInfo();

    int getPosition() const { return position_->value(); }

    int getMaximum() const { return position_->maximum(); }

    void setPosition(int position);

    void setMaximum(int maximum);

    void start();

    void stop();

    bool isLooping() const { return loop_->isChecked(); }

signals:

    void started();

    void stopped();

    void positionChanged(int position);

private slots:

    void on_position__valueChanged(int value);

    void on_startStop__clicked();

    void on_loop__toggled(bool state);
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
