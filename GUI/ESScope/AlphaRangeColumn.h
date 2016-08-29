#ifndef SIDECAR_GUI_ESSCOPE_ALPHARANGECOLUMN_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_ALPHARANGECOLUMN_H

#include "DataContainer.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class RadarSettings;

class AlphaRangeColumn : public DataContainer
{
public:
    AlphaRangeColumn() : DataContainer(), scan_(-1) {}

    void update(const Messages::Video::Ref& msg,
                const RadarSettings* radarSettings, int scan);

private:
    int scan_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
