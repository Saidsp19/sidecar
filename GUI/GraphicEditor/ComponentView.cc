#include "ComponentView.h"
#include "LinkView.h"
#include "MachineView.h"

#include <QPainter>
#include <QString>
#include <QStyleOptionGraphicsItem>

ComponentView::ComponentView(ComponentType ct, QGraphicsItem *parent) : QGraphicsItem(parent),
                                                                        subtype(ct),
                                                                        inputConnected(false),
                                                                        outputConnected(false),
                                                                        //mw(mw),
                                                                        chainup(0),
                                                                        chaindown(0)
{
    setFlags(ItemIsSelectable | ItemIsMovable);
    setAcceptsHoverEvents(true);
}

ComponentView::~ComponentView()
{
    setSource(0);
    setDrain(0);
}

QRectF ComponentView::boundingRect() const
{
    return QRectF(0, -20, boxw+2*portw, boxh+20);
}

QPainterPath ComponentView::shape() const
{
    QPainterPath path;
    path.addRect(portw, 0, boxw, boxh);
    return path;
}

void ComponentView::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);

    //QColor color=Qt::white;
    QColor fillColor;
    if (option->state & QStyle::State_MouseOver)
    {
        fillColor = Qt::lightGray;
    }
    else
    {
        fillColor = (option->state & QStyle::State_Selected) ? QColor(Qt::lightGray).light(125) : Qt::white;
    }

    if (option->levelOfDetail < 0.2)
    {
        if (option->levelOfDetail < 0.125)
        {
            painter->fillRect(QRectF(0, 0, 110, 70), fillColor);
            return;
        }

        painter->setPen(QPen(Qt::black, 0));
        painter->setBrush(fillColor);
        painter->drawRect(13, 13, 97, 57);
        return;
    }

    QPen oldPen = painter->pen();
    QPen pen = oldPen;
    int width = 0;
    if (option->state & QStyle::State_Selected)
        width += 2;

    pen.setWidth(width);
    painter->setPen(pen);
    painter->setBrush(QBrush(fillColor.dark(option->state & QStyle::State_Sunken ? 120 : 100)));
    painter->drawRect(QRect(portw, 0, boxw, boxh));

    if (option->levelOfDetail >= 0.4)
    {
        painter->setPen(oldPen);
        // Draw the input port
        QPainterPath input;
        painter->save();
        if(chainup)
            painter->setBrush(QBrush(QColor(Qt::black)));
        input.moveTo(0, (boxh-porth)/2);
        input.lineTo(portw, boxh/2);
        input.lineTo(0, (boxh+porth)/2);
        input.lineTo(0, (boxh-porth)/2);
        painter->drawPath(input);
        painter->restore();

        if (option->levelOfDetail >= 0.5)
        {
            // Draw connecting lines

            if (option->levelOfDetail >= 1)
            {
                // Draw a shadow
                painter->setPen(QPen(Qt::gray, shadowSize));
                painter->drawLine(portw+shadowOffset, boxh+shadowOffset, portw+boxw+shadowOffset, boxh+shadowOffset);
                painter->drawLine(portw+boxw+shadowOffset, shadowOffset, portw+boxw+shadowOffset, boxh+shadowOffset);

                if (option->levelOfDetail >= 1)
                {
                    // Draw text
                    painter->setPen(oldPen);
                    QFont font("Times", 10);
                    font.setStyleStrategy(QFont::ForceOutline);
                    painter->setFont(font);
                    painter->setRenderHint(QPainter::TextAntialiasing, false);
                    painter->drawText(portw+margin, lineHeight, QString("Apply 2-pulse MTI filtering."));
                    painter->drawText(portw+margin, 2*lineHeight, QString("More text description."));
                    painter->drawText(portw+margin, boxh-margin, QString("MTI2@mojave"));
                    //painter->restore();
                }

                // draw top decorations
                switch(subtype)
                {
                case CT_algorithm: // no decoration
                    break;
                case CT_publisher: // radiating antenna
                {
                    int xcenter=portw + boxw/2;
                    const int size=5;
                    const int size2=2*size;
                    const int size4=4*size;
                    // antenna
                    painter->drawLine(xcenter, 0, xcenter, -size4);
                    painter->drawLine(xcenter-size2, -size4, xcenter+size2, -size4);
                    painter->drawLine(xcenter-size2, -size4, xcenter, -size2);
                    painter->drawLine(xcenter+size2, -size4, xcenter, -size2);
                    // outgoing
                    painter->drawLine(xcenter+3*size, -4*size, xcenter+6*size, -4*size);
                    painter->drawLine(xcenter+6*size, -4*size, xcenter+5*size, -3*size);
                    painter->drawLine(xcenter+5*size, -3*size, xcenter+8*size, -3*size);
                    painter->drawLine(xcenter+7*size, -4*size, xcenter+8*size, -3*size);
                    painter->drawLine(xcenter+7*size, -2*size, xcenter+8*size, -3*size);
                }
                break;
                case CT_subscriber: // absorbing antenna
                {
                    int xcenter=portw + boxw/2;
                    const int size=5;
                    const int size2=2*size;
                    const int size4=4*size;
                    // antenna
                    painter->drawLine(xcenter, 0, xcenter, -size4);
                    painter->drawLine(xcenter-size2, -size4, xcenter+size2, -size4);
                    painter->drawLine(xcenter-size2, -size4, xcenter, -size2);
                    painter->drawLine(xcenter+size2, -size4, xcenter, -size2);
                    // incoming
                    painter->drawLine(xcenter-8*size, -4*size, xcenter-5*size, -4*size);
                    painter->drawLine(xcenter-5*size, -4*size, xcenter-6*size, -3*size);
                    painter->drawLine(xcenter-6*size, -3*size, xcenter-3*size, -3*size);
                    painter->drawLine(xcenter-4*size, -4*size, xcenter-3*size, -3*size);
                    painter->drawLine(xcenter-4*size, -2*size, xcenter-3*size, -3*size);
                }
                break;
                case CT_filereader: // arrow out of folder
                {
                    int xcenter=portw + boxw/2;
                    const int size=5;
                    const int size2=2*size;
                    const int size4=4*size;
                    // file
                    painter->drawLine(portw, 0, portw, -4*size);
                    painter->drawLine(portw, -4*size, portw+10*size, -4*size);
                    painter->drawLine(portw+10*size, -4*size, portw+10*size, -size);
                    painter->drawLine(portw+10*size, -size, portw+boxw, -size);
                    painter->drawLine(portw+boxw, -size, portw+boxw, 0);
                    // reading
                    int right=portw+boxw;
                    painter->drawLine(xcenter, 0, xcenter, -3*size);
                    painter->drawLine(xcenter, -3*size, right-11*size, -3*size);
                    painter->drawLine(right-12*size, -4*size, right-11*size, -3*size);
                    painter->drawLine(right-12*size, -2*size, right-11*size, -3*size);
                }
                break;
                case CT_filewriter: // arrow into folder
                {
                    int xcenter=portw + boxw/2;
                    const int size=5;
                    const int size2=2*size;
                    const int size4=4*size;
                    // file
                    painter->drawLine(portw, 0, portw, -4*size);
                    painter->drawLine(portw, -4*size, portw+10*size, -4*size);
                    painter->drawLine(portw+10*size, -4*size, portw+10*size, -size);
                    painter->drawLine(portw+10*size, -size, portw+boxw, -size);
                    painter->drawLine(portw+boxw, -size, portw+boxw, 0);
                    // writing
                    painter->drawLine(portw+11*size, -3*size, xcenter, -3*size);
                    painter->drawLine(xcenter, -3*size, xcenter, 0);
                    painter->drawLine(xcenter-size, -2*size, xcenter, 0);
                    painter->drawLine(xcenter+size, -2*size, xcenter, 0);
                }
                break;
                case CT_udpreader:
                    break;
                }
            }
        }

        // Draw the output port
        // Draw the input port
        QPainterPath output;
        painter->save();
        if(chaindown)
            painter->setBrush(QBrush(QColor(Qt::black)));
        output.moveTo(portw+boxw, (boxh-porth)/2);
        output.lineTo(portw+boxw+portw, boxh/2);
        output.lineTo(portw+boxw, (boxh+porth)/2);
        output.lineTo(portw+boxw, (boxh-porth)/2);
        painter->drawPath(output);
        painter->restore();
    }
}

void ComponentView::setSource(LinkView *l)
{
    if(chainup && chainup != l)
        chainup->release();

    chainup=l;
    return;
}

void ComponentView::setDrain(LinkView *l)
{
    if(chaindown && chaindown != l)
    {
        LinkView *tmp=chaindown;
        chaindown=0;
        delete(tmp);
    }

    chaindown=l;
    return;
}

QVariant ComponentView::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch(change)
    {
    case ItemPositionChange:
        updateLinks();
        break;
    default:
        break;
    };

    return QGraphicsItem::itemChange(change, value);
}

void ComponentView::updateLinks()
{
    if(chainup)
        chainup->moveEnd();
    if(chaindown)
        chaindown->moveStart();
    if(parentItem())
    {
        MachineView *tmp=qgraphicsitem_cast<MachineView *>(parentItem());
        if(tmp)
            tmp->resize();
    }
}

