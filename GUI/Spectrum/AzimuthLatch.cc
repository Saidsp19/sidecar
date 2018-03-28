#include "GUI/DoubleSetting.h"
#include "GUI/Utils.h"
#include "Utils/Utils.h"

#include "App.h"
#include "AzimuthLatch.h"
#include "Configuration.h"

using namespace SideCar::GUI::Spectrum;

AzimuthLatch::AzimuthLatch(QWidget* parent) : QWidget(parent), Ui_AzimuthLatch(), lastAzimuth_(-1.0), caught_(false)
{
    setupUi(this);
    azimuth_->setSuffix(DegreeSymbol());

    Configuration* cfg = App::GetApp()->getConfiguration();
    cfg->getAzLatchEnabled()->connectWidget(enabled_);
    cfg->getAzLatchAzimuth()->connectWidget(azimuth_);
    cfg->getAzLatchRelatch()->connectWidget(relatch_);

    connect(cfg->getAzLatchEnabled(), SIGNAL(valueChanged(bool)), SLOT(handleEnabledChanged(bool)));

    enabled_->setEnabled(true);
    azimuth_->setEnabled(true);
    relatch_->setEnabled(true);
}

bool
AzimuthLatch::check(double azimuth)
{
    if (!enabled_->isChecked() || lastAzimuth_ < 0.0) {
        lastAzimuth_ = azimuth;
        return true;
    }

    double trigger = Utils::degreesToRadians(azimuth_->value());
    bool caught = false;

    if (lastAzimuth_ > azimuth) {
        if (lastAzimuth_ - azimuth > Utils::kCircleRadians * .90) {
            if (lastAzimuth_ < trigger) {
                double tmp = azimuth + Utils::kCircleRadians;
                caught = tmp >= trigger;
            } else {
                double tmp = lastAzimuth_ - Utils::kCircleRadians;
                caught = tmp < trigger && azimuth >= trigger;
            }
        }
    } else {
        caught = lastAzimuth_ < trigger && azimuth >= trigger;
    }

    lastAzimuth_ = azimuth;

    if (caught) {
        // Caught an azimuth. Update the widget to show that this is so, and return true to let this value thru.
        //
        if (!caught_) {
            caught_ = true;
            updateCaughtIndicator();
            return true;
        }

        // If retriggering is enabled, let this one thru.
        //
        if (relatch_->isChecked()) { return true; }
    }

    //
    //
    return !caught_;
}

void
AzimuthLatch::updateCaughtIndicator()
{
    QPalette palette(enabled_->palette());
    QColor color = caught_ ? Qt::red : Qt::black;
    palette.setColor(QPalette::Active, QPalette::Text, color);
    palette.setColor(QPalette::Active, QPalette::ButtonText, color);
    palette.setColor(QPalette::Active, QPalette::WindowText, color);

    palette.setColor(QPalette::Inactive, QPalette::Text, color);
    palette.setColor(QPalette::Inactive, QPalette::ButtonText, color);
    palette.setColor(QPalette::Inactive, QPalette::WindowText, color);

    enabled_->setPalette(palette);
    enabled_->update();

    relatch_->setPalette(palette);
    relatch_->update();
}

void
AzimuthLatch::handleEnabledChanged(bool state)
{
    caught_ = false;
    updateCaughtIndicator();
}
