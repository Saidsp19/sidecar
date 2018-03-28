#ifndef SIDECAR_GUI_ASCOPE_RADARINFOWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_RADARINFOWIDGET_H

#include "GUI/RadarInfoWidget.h"

namespace SideCar {
namespace Time {
class TimeStamp;
}
namespace GUI {
namespace AScope {

class App;

/** Adaptation of GUI/InfoWidget QToolBar widget that shows the shaft encoding value of the last message
    processed by the application.
*/
class RadarInfoWidget : public ::SideCar::GUI::RadarInfoWidget {
    Q_OBJECT
    using Super = ::SideCar::GUI::RadarInfoWidget;

public:
    /** Constructor. Creates and initializes window widgets.
     */
    RadarInfoWidget(QWidget* parent);

private:
    void showMessageInfo(const Messages::PRIMessage::Ref& msg);

    bool needUpdate() const { return Super::needUpdate() || updateShaft_; }

    QString makeLabel();

    uint32_t shaft_;
    QString shaftText_;
    bool updateShaft_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
