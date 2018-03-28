#include "MachineView.h"
#include "ComponentView.h"
#include "MainWindow.h"

int boxw = 200, boxh = 200;
int portw = 10, porth = 10;

MachineView::MachineView(QGraphicsItem* parent) :
    QGraphicsItem(parent), minsize(0, -20, boxw + 2 * portw, boxh + 20), bounds(minsize)
{
    setFlags(ItemIsSelectable | ItemIsMovable);
    setAcceptsHoverEvents(true);
}

QRectF
MachineView::boundingRect() const
{
    return bounds;
}

QPainterPath
MachineView::shape() const
{
    QPainterPath path;
    path.addRect(bounds);
    return path;
}

void
MachineView::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    // QPen pen = painter->pen();
    // pen.setWidth(width);
    // painter->setPen(pen);
    painter->setBrush(QBrush(QColor(0, 128, 128, 128)));
    painter->drawRect(bounds);
}

QVariant
MachineView::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change) {
    case ItemPositionChange:
        foreach (QGraphicsItem* item, children()) {
            ComponentView* c = qgraphicsitem_cast<ComponentView*>(item);
            if (c) { c->updateLinks(); }
        }
        break;
    default: break;
    };

    return QGraphicsItem::itemChange(change, value);
}
