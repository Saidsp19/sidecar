#ifndef SIDECAR_GUI_SPECTRUM_CHANNELSELECTORWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_CHANNELSELECTORWIDGET_H

#include "ui_ChannelSelectorWidget.h"

namespace SideCar {
namespace GUI {
namespace Spectrum {

/** Widget that holds a QComboBox button that allow the user to select which Video channel to connect to for
    data, and a QCheckBox widget that indicates whether the incoming data is complex or not.
*/
class ChannelSelectorWidget : public QWidget, public Ui::ChannelSelectorWidget
{
    Q_OBJECT
public:

    /** Constructor.
     */
    ChannelSelectorWidget(QWidget* parent = 0);
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
