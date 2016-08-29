#ifndef SIDECAR_GUI_CHANNELIMAGING_H // -*- C++ -*-
#define SIDECAR_GUI_CHANNELIMAGING_H

#include "GUI/BoolSetting.h"
#include "GUI/Color.h"
#include "GUI/ColorButtonSetting.h"
#include "GUI/DoubleSetting.h"
#include "GUI/OnOffSettingsBlock.h"
#include "GUI/OpacitySetting.h"
#include "GUI/LogUtils.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

/** Collection of display attributes for a data channel or artifact. Thise base class holds separate RGB
    components, a GL point size value, and a visibility boolean. The ConfigurationWindow class contains a GUI
    analog that provides a way to change these values from the ConfigurationWindow GUI. This is the model
    component of the MVC triad.
*/
class ChannelImaging : public OnOffSettingsBlock
{
    Q_OBJECT
    using Super = OnOffSettingsBlock;
public:
    
    /** Constructor. Create a new model for a particular imaging part of the PPIWidget.

        \param enabled setting that controls whether the channel is rendered

        \param color setting that contains the color to render with

        \param size setting that controls the point or line size used during rendering

        \param opacity setting that controls the alpha value of the color used during rendering
    */
    ChannelImaging(BoolSetting* enabled, ColorButtonSetting* color, 
                   DoubleSetting* size, OpacitySetting* opacity);

    /** Obtain the current pen color setting as a Qt QColor value.

        \return QColor value
    */
    const QColor& getQColor() const { return qcolor_->getValue(); }

    /** Obtain the current pen color setting as a SideCar::GUI::Color value.

        \return SideCar::GUI::Color value
    */
    const Color& getColor() const { return color_; }

    /** Obtain the current pen point/line size.

        \return point/line size value
    */
    float getSize() const { return size_->getValue(); }

    /** Obtain the current alpha (opacity) value. Larger values increase opacity, while smaller values increase
        transparency.

        \return value in the range [0.0 - 1.0]
    */
    float getAlpha() const { return color_.alpha; }

signals:
    
    /** Notification sent out when the color setting changes.
     */
    void colorChanged();

    /** Notification sent out when the size setting changes.
     */
    void sizeChanged();

    /** Notification sent out when the size setting changes.
     */
    void alphaChanged();

protected slots:

    virtual void colorChanged(const QColor& color);

    virtual void sizeChanged(double size);

    virtual void opacityChanged(double opacity);

private:
    ColorButtonSetting* qcolor_;
    DoubleSetting* size_;
    Color color_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
