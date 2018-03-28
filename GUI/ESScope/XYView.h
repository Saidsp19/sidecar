#ifndef SIDECAR_GUI_ESSCOPE_XYVIEW_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_XYVIEW_H

#include "QtCore/QString"
#include "QtGui/QLabel"
#include "QtGui/QWidget"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace ESScope {

class RadarSettings;
class ScaleWidget;
class ViewSettings;
class XYWidget;

class XYView : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    static Logger::Log& Log();

    XYView(QWidget* parent, ViewSettings* viewSettings, const QString& title, const QString& xLabel,
           const QString& yLabel);

    double getCursorX() const { return cursorX_; }
    double getCursorY() const { return cursorY_; }

    XYWidget* getDisplay() const { return display_; }

    void setSlaveAlpha(double alpha);

public slots:

    void refresh();
    void clear();

private slots:

    void viewSettingsChanged();

    void cursorMoved(double x, double y);

    void updateGridPositions();

protected:
    virtual QString formatXValue(double value) const { return QString::number(value, 'f', 3); }

    virtual QString formatYValue(double value) const { return QString::number(value, 'f', 3); }

protected:
    void setXYWidget(XYWidget* display);

    void showEvent(QShowEvent* event);

    void resizeEvent(QResizeEvent* event);

    RadarSettings* radarSettings_;
    ViewSettings* viewSettings_;
    XYWidget* display_;
    QString title_;
    QString xLabel_;
    QString yLabel_;
    QLabel* heading_;
    ScaleWidget* xScale_;
    ScaleWidget* yScale_;
    double cursorX_;
    double cursorY_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
