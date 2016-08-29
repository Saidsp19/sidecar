#ifndef LINK_VIEW_H
#define LINK_VIEW_H

class ComponentView;

#include <QGraphicsItem>
#include <QPointF>

/**
   Draw pub/sub links or chained links
*/
class LinkView : public QGraphicsItem
{
public:
    LinkView(ComponentView *source, ComponentView *drain);
    ~LinkView();
    void release();

    // QGraphicsItem API:
    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void moveStart();
    void moveEnd();

    enum { Type = UserType + 2 };
    int type() const { return Type; }



private:
    ComponentView *src, *dst;
    QPointF srcPoint, dstPoint;
    float x0, y0, x1, y1;
};

/** \file
 */

#endif
