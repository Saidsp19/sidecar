#ifndef SIDECAR_GUI_BSCOPE_RANGEMAP_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_RANGEMAP_H

#include "GUI/RangeMap.h"

namespace SideCar {
namespace GUI {
namespace BScope {

class ViewSettings;

class RangeMap : public SideCar::GUI::RangeMap
{
    Q_OBJECT
    using Super = SideCar::GUI::RangeMap;
public:

    RangeMap();

private slots:

    void viewSettingsChanged();

private:

    void addVertex(const Vertex& vertex);

    double azimuthMin_;
    double azimuthMax_;
    Vertex last_;
    ViewSettings* viewSettings_;
};


} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
