#ifndef SIDECAR_GUI_QPOTENTIOMETER_H // -*- C++ -*-
#define SIDECAR_GUI_QPOTENTIOMETER_H

#include "QtDesigner/QDesignerExportWidget"
#include "QtGui/QWidget"

namespace SideCar {
namespace GUI {

class QDESIGNER_WIDGET_EXPORT Potentiometer : public QWidget {
    Q_OBJECT

    Q_PROPERTY(double mininum READ getMininum WRITE setMininum);
    Q_PROPERTY(double maximum READ getMaximum WRITE setMaximum);
    Q_PROPERTY(double value READ getValue WRITE setValue);
    Q_PROPERTY(double step READ getStep WRITE setStep);
    Q_PROPERTY(int tickCount READ getTickCount WRITE setTickCount);
    Q_PROPERTY(int precision READ getPrecision WRITE setPrecision);
    Q_PROPERTY(QColor startColor READ getStartColor WRITE setStartColor);
    Q_PROPERTY(QColor endColor READ getEndColor WRITE setEndColor);
    Q_PROPERTY(bool showScrews READ getShowScrews WRITE setShowScrews);
    Q_PROPERTY(bool showSweep READ getShowSweep WRITE setShowSweep);
    Q_PROPERTY(bool showValue READ getShowValue WRITE setShowValue);

public:
    Potentiometer(QWidget* parent = 0);

    QSize minimumSizeHint() const;

    QSize sizeHint() const;

    double getValue() const { return value_; }

    double getMininum() const { return minimum_; }

    double getMaximum() const { return maximum_; }

    double getStep() const { return step_; }

    int getTickCount() const { return tickCount_; }

    int getPrecision() const { return precision_; }

    QColor getStartColor() const { return startColor_; }

    QColor getEndColor() const { return endColor_; }

    bool getShowScrews() const { return showScrews_; }

    bool getShowSweep() const { return showSweep_; }

    bool getShowValue() const { return showValue_; }

signals:

    void valueChanged(double);

public slots:

    void setMininum(double value);

    void setMaximum(double value);

    void setStep(double value);

    void setValue(double value);

    void setTickCount(int value);

    void setPrecision(int value);

    void setStartColor(QColor color);

    void setEndColor(QColor color);

    void setShowScrews(bool value);

    void setShowSweep(bool value);

    void setShowValue(bool value);

protected:
    void paintEvent(QPaintEvent* event);

    void wheelEvent(QWheelEvent* event);

    void keyPressEvent(QKeyEvent* event);

private:
    void paintBorder(QPainter& painter);

    void paintKnob(QPainter& painter);

    void paintTicks(QPainter& painter);

    void paintSweep(QPainter& painter);

    void paintValue(QPainter& painter);

    double value_;
    double minimum_;
    double maximum_;
    double step_;

    int tickCount_;
    int precision_;

    QColor startColor_;
    QColor endColor_;

    bool showScrews_;
    bool showSweep_;
    bool showValue_;
};

} // namespace GUI
} // namespace SideCar

#endif
