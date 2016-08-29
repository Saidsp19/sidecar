#ifndef SIDECAR_GUI_BSCOPE_ALPHABETAWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_ALPHABETAWIDGET_H

#include "RadarSettings.h"
#include "XYWidget.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class AlphaBetaView;

class AlphaBetaWidget :  public XYWidget
{
    Q_OBJECT
    using Super = XYWidget;
public:

    static Logger::Log& Log();
    
    AlphaBetaWidget(AlphaBetaView* parent, ViewSettings* viewSettings);

    int getXScans() const { return radarSettings_->getAlphaScans(); }
    int getYScans() const { return radarSettings_->getBetaScans(); }
    double getXMinMin() const { return radarSettings_->getAlphaMinMin(); }
    double getXMaxMax() const { return radarSettings_->getAlphaMaxMax(); }
    double getYMinMin() const { return radarSettings_->getBetaMinMin(); }
    double getYMaxMax() const { return radarSettings_->getBetaMaxMax(); }

private slots:

    void currentMessage(const Messages::PRIMessage::Ref& msg);

private:

    void alphasChanged(const AlphaIndices& indices);

    void paintGL();

    void updateColumn(int alphaIndex);

    void fillColors();

    double lastAlpha_;
    double lastBeta_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
