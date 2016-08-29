#ifndef SIDECAR_GUI_QLED_H	// -*- C++ -*-
#define SIDECAR_GUI_QLED_H

#include "QtGui/QWidget"
#include "QtDesigner/QDesignerExportWidget"

class QColor;

class QDESIGNER_WIDGET_EXPORT QLed : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool value READ getValue WRITE setValue);
    Q_PROPERTY(bool pending READ getPending WRITE setPending);
    Q_PROPERTY(QColor color READ getColor WRITE setColor);

public: 

    QLed(QWidget* parent = 0);

    bool getValue() const { return value_; }

    bool getPending() const { return pending_; }

    const QColor& getColor() const { return color_; }

public slots:

    void setValue(bool value);

    void setPending(bool value);

    void setColor(const QColor& color);

    void toggleValue();

protected:

    void paintEvent(QPaintEvent* event);

private:
    bool value_;
    bool pending_;
    QColor color_;
};

#endif
