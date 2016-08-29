#ifndef SIDECAR_GUI_ASCOPE_VIEWSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_VIEWSETTINGS_H

#include "QtCore/QMetaType"
#include "QtCore/QRectF"

class QSettings;

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace AScope {

class ViewBounds {

public:

    ViewBounds() {}

    ViewBounds(double xMin, double xMax, double yMin, double yMax)
	: xMin_(xMin), xMax_(xMax), yMin_(yMin), yMax_(yMax) {}

    double getXMin() const { return xMin_; }
    double getXMax() const { return xMax_; }

    double getYMin() const { return yMin_; }
    double getYMax() const { return yMax_; }

    double getWidth() const { return xMax_ - xMin_; }
    double getHeight() const { return yMax_ - yMin_; }

    void translate(double dx, double dy)
	{ xMin_ += dx; xMax_ += dx; yMin_ += dy; yMax_ += dy; }

private:
    
    double xMin_, xMax_, yMin_, yMax_;
};

/** User-defined view setting.
 */
class ViewSettings
{
public:

    static Logger::Log& Log();

    ViewSettings() {}

    ViewSettings(double rangeMin, double rangeMax,
                 int gateMin, int gateMax, double voltageMin,
                 double voltageMax, int sampleMin, int sampleMax,
                 bool showingRanges, bool showingVoltages);

    ViewSettings(QSettings& settings);

    const ViewBounds& getBounds() const { return bounds_; }

    void setBounds(const ViewBounds& bounds);

    double getRangeMin() const { return rangeMin_; }

    double getRangeMax() const { return rangeMax_; }

    int getGateMin() const { return gateMin_; }

    int getGateMax() const { return gateMax_; }

    double getVoltageMin() const { return voltageMin_; }

    double getVoltageMax() const { return voltageMax_; }

    int getSampleMin() const { return sampleMin_; }

    int getSampleMax() const { return sampleMax_; }

    bool isShowingRanges() const { return showingRanges_; }

    void setShowingRanges(bool state);

    bool isShowingVoltages() const { return showingVoltages_; }

    void setShowingVoltages(bool state);

    void restoreFromSettings(QSettings& settings);

    void saveToSettings(QSettings& settings) const;

    bool operator==(const ViewSettings& rhs) const;

    bool operator!=(const ViewSettings& rhs) const
	{ return ! operator==(rhs); }

private:

    void updateBounds();

    ViewBounds bounds_;
    double rangeMin_;
    double rangeMax_;
    int gateMin_;
    int gateMax_;
    double voltageMin_;
    double voltageMax_;
    int sampleMin_;
    int sampleMax_;
    bool showingRanges_;
    bool showingVoltages_;

    static int const kMetaTypeId;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

Q_DECLARE_METATYPE(SideCar::GUI::AScope::ViewSettings)

#endif
