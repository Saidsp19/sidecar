#ifndef SIDECAR_GUI_ESSCOPE_ALPHABETAVIEW_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_ALPHABETAVIEW_H

#include "RadarSettings.h"
#include "ViewSettings.h"
#include "XYView.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class AlphaBetaWidget;

class AlphaBetaView : public XYView
{
    Q_OBJECT
    using Super = XYView;
public:

    AlphaBetaView(QWidget* parent, ViewSettings* viewSettings);

    AlphaBetaWidget* getDisplay() const;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
