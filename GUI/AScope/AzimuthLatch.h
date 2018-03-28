#ifndef SIDECAR_GUI_ASCOPE_AZIMUTHLATCH_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_AZIMUTHLATCH_H

#include "QtCore/QStringList"

#include "GUI/OnOffSettingsBlock.h"
#include "GUI/QCheckBoxSetting.h"
#include "Utils/Utils.h"

#include "ui_AzimuthLatch.h"

namespace SideCar {
namespace GUI {

namespace AScope {

class AzimuthLatch : public QWidget, private Ui_AzimuthLatch {
    Q_OBJECT
    using Super = QWidget;

public:
    AzimuthLatch(QWidget* parent = 0);

    void setConfiguration(bool enabled, double azimuth, bool relatch, bool caught, const QStringList& channels,
                          const QString& active);

    bool isEnabled() const { return enabled_->isChecked(); }

    double getAzimuth() const { return azimuth_->value(); }

    bool isRelatching() const { return relatch_->isChecked(); }

    void reset();

    void latch();

signals:

    void configurationChanged(bool enabled, double azimuth, bool relatch, const QString& channel);

private slots:

    void handleChange();

private:
    void updateCaughtIndicator(bool caught);

    double lastAzimuth_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

#endif
