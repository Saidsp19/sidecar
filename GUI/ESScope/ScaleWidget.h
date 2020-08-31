#ifndef SIDECAR_GUI_ESSCOPE_SCALEWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_SCALEWIDGET_H

#include "QtCore/QLineF"
#include "QtCore/QPointF"
#include "QtCore/QVector"

#include "QtGui/QPen"
#include "QtGui/QTransform"
#include "QtWidgets/QWidget"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace ESScope {

/** Widget that draws tick marks and value labels. Supports horizontal and vertical drawing. Also draws grid
    lines and an triangle indicator showing horizontal or vertical position of the cursor.
*/
class ScaleWidget : public QWidget {
    using Super = QWidget;
    Q_OBJECT
public:
    static Logger::Log& Log();

    ScaleWidget(QWidget* parent = 0, Qt::Orientation orientation = Qt::Horizontal);

    void setLabel(const QString& label);

    QSize sizeHint() const;

    // QSize minimumSizeHint() const;

    void setSpan(int span);

    int getSpan() const { return orientation_ == Qt::Horizontal ? width() : height(); }

    void setAutoDivide(bool autoDivide);
    bool getAutoDivide() const { return autoDivide_; }

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
    void setStartAndEnd(double start, double end);
    void setOrientation(Qt::Orientation orientation);
    void setMajorGridPen(QPen pen) { majorGridPen_ = pen; }
    void setMinorGridPen(QPen pen) { minorGridPen_ = pen; }

    void drawGridLines(QPainter& paintDevice);

    void setCursorPosition(int position);

    void setCursorValue(double value);

    double getValueAt(int index) const { return range_ * index / double(getSpan() - 1) + start_; }

    virtual QString formatTickTag(double value) { return QString::number(value); }

    std::vector<float> getMajorTickPositions() const;

    std::vector<float> getMinorTickPositions() const;

private:
    void resizeEvent(QResizeEvent* event);
    void paintEvent(QPaintEvent* event);

    void recalculateTickIntervals();
    void drawGridLines(QPainter& painter, int height) const;

    void makeTicks();

    double getTickSpan() const;
    double getTagOffset() const;

    QString label_;
    QPoint labelPosition_;

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

    QVector<QLineF> majorTicks_;
    QVector<QLineF> minorTicks_;
    QVector<QPointF> labelStarts_;
    QVector<QString> labelTexts_;

    QTransform transform_;

    int cursorPosition_;
    bool autoDivide_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
