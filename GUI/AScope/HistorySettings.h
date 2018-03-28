#ifndef SIDECAR_GUI_ASCOPE_HISTORYSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_HISTORYSETTINGS_H

#include "GUI/IntSetting.h"
#include "GUI/OnOffSettingsBlock.h"

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {
namespace AScope {

/** Collection of settings that configure the application's History object.
 */
class HistorySettings : public OnOffSettingsBlock {
    Q_OBJECT
    using Super = OnOffSettingsBlock;

public:
    /** Obtain Log device for ViewSetting objects

        \return Log device
    */
    static Logger::Log& Log();

    /** Constructor.
     */
    HistorySettings(BoolSetting* enabled, IntSetting* duration);

    int getDuration() const { return duration_->getValue(); }

signals:

    void durationChanged(int duration);

private:
    IntSetting* duration_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
