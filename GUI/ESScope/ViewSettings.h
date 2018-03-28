#ifndef SIDECAR_GUI_ESSCOPE_VIEWSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_VIEWSETTINGS_H

#include <vector>

#include "QtCore/QObject"

class QSettings;

namespace SideCar {
namespace GUI {
namespace ESScope {

class RadarSettings;

struct ViewBounds {
    ViewBounds() {}

    ViewBounds(double x1, double x2, double y1, double y2) : xMin(x1), xMax(x2), yMin(y1), yMax(y2) {}

    bool viewingX(double value) const { return value >= xMin && value <= xMax; }

    bool viewingY(double value) const { return value >= yMin && value <= yMax; }

    bool operator==(const ViewBounds& rhs) const
    {
        return xMin == rhs.xMin && xMax == rhs.xMax && yMin == rhs.yMin && yMax == rhs.yMax;
    }

    bool operator!=(const ViewBounds& rhs) const { return !operator==(rhs); }

    double xMin, xMax, yMin, yMax;
};

class ViewSettings : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    using ViewBoundsStack = std::vector<ViewBounds>;

    void setViewBounds(const ViewBounds& viewBounds);

    void push(const ViewBounds& viewBounds);

    void push(double xMin, double xMax, double yMin, double yMax) { push(ViewBounds(xMin, xMax, yMin, yMax)); }

    const ViewBounds& getViewBounds() const { return stack_.back(); }

    double getXMin() const { return stack_.back().xMin; }

    double getXMax() const { return stack_.back().xMax; }

    double getXSpan() const { return getXMax() - getXMin(); }

    double getYMin() const { return stack_.back().yMin; }

    double getYMax() const { return stack_.back().yMax; }

    double getYSpan() const { return getYMax() - getYMin(); }

    bool viewingX(double x) const { return stack_.back().viewingX(x); }

    bool viewingY(double y) const { return stack_.back().viewingY(y); }

    bool canPop() const { return stack_.size() > 1 || stack_.back() != home_; }

signals:

    void viewChanged();

public slots:

    void pop();

    void popAll();

    void swap();

private slots:

    void radarSettingsChanged();

protected:
    ViewSettings(QObject* parent, const RadarSettings* radarSettings, const ViewBounds& initView);

    const RadarSettings* radarSettings_;

private:
    virtual bool updateViewBounds(ViewBounds& viewBounds) = 0;

    ViewBoundsStack stack_;
    ViewBounds home_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
