#include "GUI/Utils.h"
#include "Utils/Utils.h"

#include "App.h"
#include "AzimuthLatch.h"
#include "Configuration.h"

using namespace SideCar::GUI::AScope;

AzimuthLatch::AzimuthLatch(QWidget* parent) : QWidget(parent), Ui_AzimuthLatch(), lastAzimuth_(-1.0)
{
    setupUi(this);
    azimuth_->setSuffix(DegreeSymbol());
    connect(enabled_, SIGNAL(clicked(bool)), SLOT(handleChange()));
    connect(channel_, SIGNAL(activated(const QString&)), SLOT(handleChange()));
    connect(azimuth_, SIGNAL(valueChanged(double)), SLOT(handleChange()));
    connect(relatch_, SIGNAL(clicked(bool)), SLOT(handleChange()));
}

void
AzimuthLatch::updateCaughtIndicator(bool caught)
{
    QPalette palette(enabled_->palette());
    QColor color = caught ? Qt::red : Qt::black;
    palette.setColor(QPalette::Active, QPalette::Text, color);
    palette.setColor(QPalette::Active, QPalette::ButtonText, color);
    palette.setColor(QPalette::Active, QPalette::WindowText, color);

    palette.setColor(QPalette::Inactive, QPalette::Text, color);
    palette.setColor(QPalette::Inactive, QPalette::ButtonText, color);
    palette.setColor(QPalette::Inactive, QPalette::WindowText, color);

    enabled_->setPalette(palette);
    enabled_->update();

    channel_->setPalette(palette);
    channel_->update();

    relatch_->setPalette(palette);
    relatch_->update();
}

void
AzimuthLatch::handleChange()
{
    emit configurationChanged(enabled_->isChecked(), azimuth_->value(), relatch_->isChecked(), channel_->currentText());
}

void
AzimuthLatch::setConfiguration(bool enabled, double azimuth, bool relatch, bool caught, const QStringList& names,
                               const QString& active)
{
    channel_->clear();
    channel_->addItems(names);
    channel_->setCurrentIndex(names.indexOf(active));
    enabled_->setChecked(enabled);
    azimuth_->setValue(azimuth);
    relatch_->setChecked(relatch);
    updateCaughtIndicator(caught);
}

void
AzimuthLatch::reset()
{
    updateCaughtIndicator(false);
}

void
AzimuthLatch::latch()
{
    updateCaughtIndicator(true);
}
