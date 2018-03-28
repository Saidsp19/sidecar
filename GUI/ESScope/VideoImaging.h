#ifndef SIDECAR_GUI_ESSCOPE_VIDEOIMAGING_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_VIDEOIMAGING_H

#include "QtGui/QImage"

#include "GUI/CLUT.h"

#include "SampleImaging.h"

namespace SideCar {
namespace GUI {

class CLUTSetting;

namespace ESScope {

/** Extension of the SampleImaging class for the display of Video data. Contains a color lookup table (CLUT)
    object that may be used for false-color imaging.
*/
class VideoImaging : public SampleImaging {
    Q_OBJECT
    using Super = SampleImaging;

public:
    enum { kRed, kGreen, kBlue, kRedGreen, kRedBlue, kGreenBlue };

    VideoImaging(BoolSetting* enabled, ColorButtonSetting* color, DoubleSetting* pointSize, OpacitySetting* alpha,
                 QComboBoxSetting* decimation, BoolSetting* colorMapEnabled, CLUTSetting* colorMapType);

    bool getColorMapEnabled() const { return colorMapEnabled_->getValue(); }

    Color getColor(double intensity) const;

    const QImage& getColorMap() const { return colorMap_; }

signals:

    /** Notification sent out when the rendering colormap changes.

        \param colorMap representation of new colormap
    */
    void colorMapChanged(const QImage& colorMap);

private slots:

    /** Notification handler invoked when the colormap enabled setting changes state.

        \param value new setting value
    */
    void setColorMapEnabled(bool value);

    /** Notification handler invoked when the colormap selection setting changes.

        \param value new index value
    */
    void setColorMapType(int value);

private:
    /** Create a new colormap QImage representation, and emit the colorMapChanged() signal.
     */
    void updateColorMapImage();

    BoolSetting* colorMapEnabled_;
    QImage colorMap_;
    CLUT clut_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
