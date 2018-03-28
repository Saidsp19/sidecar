#ifndef SIDECAR_GUI_SPECTRUM_SPECTROGRAPHIMAGING_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_SPECTROGRAPHIMAGING_H

#include "QtGui/QImage"

#include "GUI/CLUT.h"
#include "GUI/Color.h"
#include "GUI/QComboBoxSetting.h"
#include "GUI/QDoubleSpinBoxSetting.h"

#include "SampleImaging.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Spectrum {

class SpectrographImaging : public ChannelImaging {
    Q_OBJECT
    using Super = ChannelImaging;

public:
    enum { kRed, kGreen, kBlue, kRedGreen, kRedBlue, kGreenBlue };

    static Logger::Log& Log();

    SpectrographImaging(BoolSetting* enabled, ColorButtonSetting* color, DoubleSetting* pointSize,
                        OpacitySetting* opacity, BoolSetting* colorMapEnabled, QComboBoxSetting* colorMapType,
                        QDoubleSpinBoxSetting* minCutoff, QDoubleSpinBoxSetting* maxCutoff, IntSetting* historySize);

    bool getColorMapEnabled() const { return colorMapEnabled_->getValue(); }

    Color getColor(double db) const;

    const QImage& getColorMap() const { return colorMap_; }

    double getMinCutoff() const { return minCutoff_->getValue(); }

    double getMaxCutoff() const { return maxCutoff_->getValue(); }

    void setMinCutoff(double value) { minCutoff_->setValue(value); }

    void setMaxCutoff(double value) { maxCutoff_->setValue(value); }

    int getHistorySize() const { return historySize_->getValue(); }

    QComboBox* duplicateColorMapType(QWidget* parent = 0) const { return colorMapType_->duplicate(parent); }

    QDoubleSpinBox* duplicateMinCutoff(QWidget* parent = 0) const { return minCutoff_->duplicate(parent); }

    QDoubleSpinBox* duplicateMaxCutoff(QWidget* parent = 0) const { return maxCutoff_->duplicate(parent); }

    void setColorMapIndex(int value) { colorMapType_->setValue(value); }

signals:

    /** Notification sent out when the rendering colormap changes.

        \param colorMap representation of new colormap
    */
    void colorMapChanged(const QImage& colorMap);

    void minCutoffChanged(double value);

    void maxCutoffChanged(double value);

    void historySizeChanged(int value);

private slots:

    /** Notification handler invoked when the colormap enabled setting changes state.

        \param value new setting value
    */
    void setColorMapEnabled(bool value);

    /** Notification handler invoked when the colormap selection setting changes.

        \param value new index value
    */
    void setColorMapType(int value);

    void updateDbColorTransform();

private:
    /** Create a new colormap QImage representation, and emit the colorMapChanged() signal.
     */
    void updateColorMapImage();

    BoolSetting* colorMapEnabled_;
    QImage colorMap_;
    CLUT clut_;
    QComboBoxSetting* colorMapType_;
    QDoubleSpinBoxSetting* minCutoff_;
    QDoubleSpinBoxSetting* maxCutoff_;
    IntSetting* historySize_;
    double offset_;
    double scaling_;
};

} // namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
