#include "LinkView.h"
#include "ComponentView.h"

#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>

LinkView::LinkView(ComponentView* s, ComponentView* d) :
    src(s), dst(d), srcPoint(mapFromItem(src, src->drainPoint(this)))
{
    src->setDrain(this);
    dst->setSource(this);

    moveEnd();

    setFlags(ItemIsSelectable);
    setAcceptsHoverEvents(true);
}

// forcibly disconnect from the other end
LinkView::~LinkView()
{
    src->setDrain(0);
    dst->setSource(0);
}
void
LinkView::release()
{
    src->setDrain(0);
}

QRectF
LinkView::boundingRect() const
{
    return QRectF(x0 - 10, y0 - 10, x1 - x0 + 20, y1 - y0 + 20);
}

QPainterPath
LinkView::shape() const
{
    QPainterPath path;
    path.moveTo(srcPoint);
    path.lineTo(dstPoint);
    return path;
}

void
LinkView::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (option->state & QStyle::State_Selected) {
        QPen pen = painter->pen();
        pen.setWidth(2);
        painter->setPen(pen);
    }

    painter->drawLine(srcPoint, dstPoint);
}

void
LinkView::moveStart()
{
    srcPoint = mapFromItem(src, src->drainPoint(this));

    prepareGeometryChange();

    x0 = srcPoint.x();
    y0 = srcPoint.y();
    x1 = dstPoint.x();
    y1 = dstPoint.y();
    if (x0 > x1) {
        x0 = x1;
        x1 = srcPoint.x();
    }
    if (y0 > y1) {
        y0 = y1;
        y1 = srcPoint.y();
    }
}

void
LinkView::moveEnd()
{
    dstPoint = mapFromItem(dst, dst->sourcePoint(this));

    prepareGeometryChange();

    x0 = srcPoint.x();
    y0 = srcPoint.y();
    x1 = dstPoint.x();
    y1 = dstPoint.y();
    if (x0 > x1) {
        x0 = x1;
        x1 = srcPoint.x();
    }
    if (y0 > y1) {
        y0 = y1;
        y1 = srcPoint.y();
    }
}
