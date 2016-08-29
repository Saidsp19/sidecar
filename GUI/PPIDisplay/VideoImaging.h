#ifndef SIDECAR_GUI_PPIDISPLAY_VIDEOIMAGING_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_VIDEOIMAGING_H

#include "QtGui/QImage"

#include "GUI/CLUT.h"
#include "GUI/IntSetting.h"
#include "GUI/SampleImaging.h"

namespace SideCar {
namespace GUI {

class CLUTSetting;

namespace PPIDisplay {

/** Extension of the SampleImaging class for the display of Video data. Contains a color lookup table (CLUT)
    object that may be used for false-color imaging.
*/
class VideoImaging : public SampleImaging
{
    Q_OBJECT
    using Super = SampleImaging;
public:

    enum {
	kRed,
	kGreen,
	kBlue,
	kRedGreen,
	kRedBlue,
	kGreenBlue
    };

    VideoImaging(BoolSetting* enabled, ColorButtonSetting* color,
                 DoubleSetting* pointSize, OpacitySetting* alpha,
                 QComboBoxSetting* decimation, BoolSetting* colorMapEnabled,
                 CLUTSetting* clutSetting, BoolSetting* desaturateEnabled,
                 IntSetting* desaturateRate);

    bool getColorMapEnabled() const { return colorMapEnabled_->getValue(); }

    const CLUT& getCLUT() const { return clut_; }

    bool getDesaturateEnabled() const
	{ return clut_.hasSaturation() && desaturateEnabled_->getValue(); }

    double getDesaturateRate() const
	{ return desaturateRate_->getValue() / 100.0; }

    Color getColor(double intensity) const;

    const QImage& getColorMap() const { return colorMap_; }

signals:

    /** Notification sent out when the rendering colormap changes.

        \param colorMap representation of new colormap
    */
    void colorMapChanged(const QImage& colorMap);

    /** Notification sent out when a setting related to color intensity desaturation changes.
     */
    void desaturationChanged();

    void desaturationEnabledChanged();

public slots:

    void setColorMapType(int index);

private slots:

    /** Notification handler invoked when the colormap enabled setting changes state.

        \param value new setting value
    */
    void setColorMapEnabled(bool value);
    
    /** Notification handler invoked when the colormap selection setting changes.

        \param value new index value
    */
    void colorMapTypeChanged(int value);
    
    /** Notification handler invoked when the desaturate enabled setting changes state.

        \param value new setting value
    */
    void setDesaturateEnabled(bool value);
    
    /** Notification handler invoked when the desaturate rate setting changes.

        \param value new setting value
    */
    void setDesaturateRate(int value);

private:
    
    /** Create a new colormap QImage representation, and emit the colorMapChanged() signal.
     */
    void updateColorMapImage();

    
    BoolSetting* colorMapEnabled_;
    CLUTSetting* clutSetting_;
    BoolSetting* desaturateEnabled_;
    IntSetting* desaturateRate_;
    QImage colorMap_;
    CLUT clut_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
