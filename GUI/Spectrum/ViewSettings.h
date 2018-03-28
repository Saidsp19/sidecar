#ifndef SIDECAR_GUI_SPECTRUM_VIEWSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_VIEWSETTINGS_H

#include "QtCore/QMetaType"
#include "QtCore/QRectF"

class QSettings;

namespace SideCar {
namespace GUI {
namespace Spectrum {

/** User-defined view setting.
 */
class ViewSettings {
public:
    ViewSettings() {}

    ViewSettings(double xMin, double xMax, double yMin, double yMax);

    ViewSettings(QSettings& settings);

    const QRectF& getBounds() const { return bounds_; }

    void setBounds(const QRectF& bounds);

    double getXMin() const { return xMin_; }

    double getXMax() const { return xMax_; }

    double getYMin() const { return yMin_; }

    double getYMax() const { return yMax_; }

    void restoreFromSettings(QSettings& settings);

    void saveToSettings(QSettings& settings) const;

    bool operator==(const ViewSettings& rhs) const;

    bool operator!=(const ViewSettings& rhs) const { return !operator==(rhs); }

private:
    void updateBounds();

    QRectF bounds_;
    double xMin_;
    double xMax_;
    double yMin_;
    double yMax_;

    static int const kMetaTypeId;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

Q_DECLARE_METATYPE(SideCar::GUI::Spectrum::ViewSettings)

#endif
