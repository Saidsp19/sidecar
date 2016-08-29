#ifndef SIDECAR_GUI_ASCOPE_PEAKBARSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_PEAKBARSETTINGS_H

#include "GUI/IntSetting.h"
#include "GUI/OnOffSettingsBlock.h"

namespace Logger { class Log; }
namespace SideCar {
namespace GUI {
namespace AScope {

/** Collection of settings that configure the application's History object.
 */
class PeakBarSettings : public OnOffSettingsBlock
{
    Q_OBJECT
    using Super = OnOffSettingsBlock;
public:

    /** Obtain Log device for ViewSetting objects

        \return Log device
    */
    static Logger::Log& Log();

    /** Constructor.
     */
    PeakBarSettings(BoolSetting* enabled, IntSetting* width,
                    IntSetting* lifeTime, BoolSetting* fade);

    int getWidth() const { return width_; }

    int getLifeTime() const { return lifeTime_; }

    bool isFading() const { return fading_; }

signals:

    void widthChanged(int value);

    void lifeTimeChanged(int value);

    void fadingChanged(bool value);

private slots:

    void widthChange(int value);

    void lifeTimeChange(int value);

    void fadingChange(bool value);

private:
    int width_;
    int lifeTime_;
    bool fading_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
