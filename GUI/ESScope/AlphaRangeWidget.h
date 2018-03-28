#ifndef SIDECAR_GUI_BSCOPE_ALPHARANGEWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_ALPHARANGEWIDGET_H

#include "RadarSettings.h"
#include "XYWidget.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class AlphaRangeView;

class AlphaRangeWidget : public XYWidget {
    Q_OBJECT
    using Super = XYWidget;

public:
    static Logger::Log& Log();

    AlphaRangeWidget(AlphaRangeView* parent, ViewSettings* viewSettings);

    int getXScans() const { return radarSettings_->getAlphaScans(); }
    int getYScans() const { return radarSettings_->getRangeScans(); }
    double getXMinMin() const { return radarSettings_->getAlphaMinMin(); }
    double getXMaxMax() const { return radarSettings_->getAlphaMaxMax(); }
    double getYMinMin() const { return radarSettings_->getRangeMinMin(); }
    double getYMaxMax() const { return radarSettings_->getRangeMaxMax(); }

private:
    void alphasChanged(const AlphaIndices& indices);

    void paintGL();

    void updateColumn(int alphaIndex);

    void fillColors();
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
