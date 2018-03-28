#ifndef SIDECAR_GUI_BSCOPE_CURSORPOSITION_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_CURSORPOSITION_H

#include "QtCore/QObject"
#include "QtCore/QPointF"
#include "QtCore/QString"

#include "boost/shared_ptr.hpp"

namespace SideCar {
namespace GUI {
namespace BScope {

class CursorPosition : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    CursorPosition(double x, double y, double azimuth, double range);

    ~CursorPosition();

    double getX() const;
    double getY() const;
    QPointF getXY() const { return QPointF(getX(), getY()); }

    double getAzimuth() const;
    double getRange() const;

    void setSampleValue(const QString& sampleValue);
    void clearCache();

    const QString& getToolTip() const;
    const QString& getWidgetText() const;

private:
    struct Private;
    boost::shared_ptr<Private> p_;
};

} // namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
