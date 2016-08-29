#include "PlayerPositioner.h"
#include "PlayerSettings.h"

using namespace SideCar::GUI::BScope;

PlayerPositioner::PlayerPositioner(PlayerSettings* settings, QWidget* parent)
    : Super(parent), Ui::PlayerPositioner()
{
    setupUi(this);
    position_->setValue(0);
    position_->setMaximum(0);
    startStop_->setToolTip("Start playback");
    startStop_->setIconSize(QSize(16, 16));
    on_loop__toggled(loop_->isChecked());
    settings->connectPlayer(playbackRate_, loop_);
    updateButtons();
}

void
PlayerPositioner::setPosition(int position)
{
    position_->setValue(position);
}

void
PlayerPositioner::setMaximum(int maximum)
{
    position_->setMaximum(maximum);
    updateButtons();
}

void
PlayerPositioner::start()
{
    startStop_->setText("Stop");
    startStop_->setToolTip("Stop playback");
    startStop_->setChecked(true);
}

void
PlayerPositioner::stop()
{
    startStop_->setText("Play");
    startStop_->setToolTip("Start playback");
    startStop_->setChecked(false);
}
    
void
PlayerPositioner::updateInfo()
{
    int index = position_->value() + 1;
    info_->setText(QString("T-%1").arg(index));
}

void
PlayerPositioner::updateButtons()
{
    bool enabled = position_->maximum() > 0;
    startStop_->setEnabled(enabled);
    position_->setEnabled(enabled);
}

void
PlayerPositioner::on_startStop__clicked()
{
    if (startStop_->isChecked()) {
	start();
	emit started();
    }
    else {
	stop();
	emit stopped();
    }
}

void
PlayerPositioner::on_position__valueChanged(int value)
{
    updateInfo();
    emit positionChanged(value);
}

void
PlayerPositioner::on_loop__toggled(bool state)
{
    loop_->setToolTip(state ? "Clear to stop playback at last frame" :
                      "Set to loop around at last frame");
}
