#ifndef MACHINE_VIEW_H
#define MACHINE_VIEW_H

#include "MainWindow.h"
#include <QGraphicsItem>
#include <QPainterPath>

/**
 */
class MachineView : public QGraphicsItem {
public:
    MachineView(QGraphicsItem* parent = 0);

    QRectF boundingRect() const;
    QPainterPath shape() const;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    QVariant itemChange(GraphicsItemChange change, const QVariant& value);

    void resize()
    {
        prepareGeometryChange();
        bounds = minsize | childrenBoundingRect();
    }

    enum { Type = UserType + 3 };
    int type() const { return Type; }

private:
    QString hostname;
    QRectF minsize, bounds;
};

/** \file
 */

#endif
