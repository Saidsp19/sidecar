#ifndef SIDECAR_GUI_SAMPLEIMAGING_H // -*- C++ -*-
#define SIDECAR_GUI_SAMPLEIMAGING_H

#include "GUI/ChannelImaging.h"
#include "GUI/QComboBoxSetting.h"

namespace SideCar {
namespace GUI {

/** Derivation of the ChannelImaging settings collection class that contains additional settings for OpenGL
    texture size and sample decimation. The OffscreenBuffer class uses an instance of this to perform its
    rendering.
*/
class SampleImaging : public ChannelImaging {
    Q_OBJECT
    using Super = ChannelImaging;

public:
    /** Constructor.

        \param enabled

        \param color

        \param pointSize

        \param alpha

        \param textureSize

        \param decimation
    */
    SampleImaging(BoolSetting* enabled, ColorButtonSetting* color, DoubleSetting* pointSize, OpacitySetting* alpha,
                  QComboBoxSetting* decimation);

    /** Obtain the current decimation setting value.

        \return decimation value
    */
    int getDecimation() const { return decimation_->getValue() + 1; }

signals:

    /** Notification emitted when the decimation setting changes. Note that SettingsBlock::settingChanged() is
        \b not emitted.

        \param value new setting value
    */
    void decimationChanged(int value);

private:
    QComboBoxSetting* decimation_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
