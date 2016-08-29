#ifndef SIDECAR_GUI_ESSCOPE_ALPHARANGEVIEW_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_ALPHARANGEVIEW_H

#include "XYView.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class ViewSettings;

class AlphaRangeWidget;

class AlphaRangeView : public XYView
{
    Q_OBJECT
    using Super = XYView;
public:

    AlphaRangeView(QWidget* parent, ViewSettings* viewSettings);

    AlphaRangeWidget* getDisplay() const;

private:

    QString formatYValue(double value) const
	{ return Super::formatYValue(value) + " km"; }
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
