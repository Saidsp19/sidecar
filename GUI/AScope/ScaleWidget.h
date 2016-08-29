#ifndef SIDECAR_GUI_ASCOPE_SCALEWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_SCALEWIDGET_H

#include "QtGui/QPen"
#include "QtGui/QWidget"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace AScope {

/** Widget that draws tick marks and value labels. Supports horizontal and vertical drawing. Also draws grid
    lines and an triangle indicator showing horizontal or vertical position of the cursor.
*/
class ScaleWidget : public QWidget
{
    Q_OBJECT
public:
    static Logger::Log& Log();

    ScaleWidget(QWidget* parent = 0,
                Qt::Orientation orientation = Qt::Horizontal);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    int getMajorTickHeight() const { return majorTickHeight_; }
    int getMinorTickHeight() const { return minorTickHeight_; }
    int getMajorTickDivisions() const { return majorTickDivisions_; }
    int getMinorTickDivisions() const { return minorTickDivisions_; }

    double getStart() const { return start_; }
    double getEnd() const { return start_ + range_; }

    Qt::Orientation getOrientation() const { return orientation_; }
    QPen getMajorGridPen() const { return majorGridPen_; }
    QPen getMinorGridPen() const { return minorGridPen_; }

    void setMajorTickHeight(int height);
    void setMinorTickHeight(int height);
    void setMajorTickDivisions(int divisions);
    void setMinorTickDivisions(int divisions);
    void setMidPoint(double zero);
    void setRange(double range);
    void setStart(double start);
    void setEnd(double end);
    void setOrientation(Qt::Orientation orientation);
    void setMajorGridPen(QPen pen) { majorGridPen_ = pen; }
    void setMinorGridPen(QPen pen) { minorGridPen_ = pen; }

    void drawGridLines(QPainter& paintDevice);

    void setCursorPosition(int position);

    void render(QPainter& painter, int width, int height);

private:
    void recalculateTickIntervals(int width, int height);
    void paintEvent(QPaintEvent* event);
    void drawHorizontalGridLines(QPainter& painter, int width, int height)
	const;
    void drawVerticalGridLines(QPainter& painter, int width, int height)
	const;
    void dirtyCursorPosition(int position);
    void drawCursorPosition(QPainter& painter, int position);
    void drawTicks(QPainter& painter, int width, double y, int sign,
                   int tagOfffset);

    QSize lastSizeCalculated_;

    int majorTickHeight_;
    int minorTickHeight_;

    int majorTickDivisionsMax_;
    int minorTickDivisionsMax_;

    int majorTickDivisions_;
    int minorTickDivisions_;

    double majorIncrement_;
    double minorIncrement_;

    double start_;
    double range_;

    Qt::Orientation orientation_;

    QPen majorGridPen_;
    QPen minorGridPen_;
    QPen cursorIndicatorPen_;

    int cursorPosition_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
