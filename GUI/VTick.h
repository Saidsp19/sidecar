#ifndef SIDECAR_GUI_VTICK_H // -*- C++ -*-
#define SIDECAR_GUI_VTICK_H

#include "boost/shared_ptr.hpp"

#include "QtGui/QLabel"

namespace SideCar {
namespace GUI {

/** Perhaps the silliest class in all of SideCar. Represents a 2-pixel vertical tick mark. Only used by
    ColorMapWidget, so perhaps it should be rolled into it.
*/
class VTick : public QLabel
{
    Q_OBJECT
public:

    /** Constructor. Creates and installs the pixmap representing the tick.

        \param parent the parent widget of this one 
    */
    VTick(QWidget* parent = 0);

private:
    static QPixmap* tick_;	///< Tick representation common to all VTicks
};

} // end namespace GUI
} // end namespace SideCar

#endif
