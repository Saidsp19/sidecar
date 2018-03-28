#ifndef COMPONENT_VIEW_H
#define COMPONENT_VIEW_H

#include <QGraphicsItem>
#include <QList>
#include <QPainterPath>

class MainWindow;
class LinkView;

/**
   A ComponentView controls an Algorithm, pub, sub, file reader, file writer, or UDP reader.
*/
class ComponentView : public QGraphicsItem {
public:
    enum ComponentType {
        CT_algorithm = 0, // default
        CT_publisher,
        CT_subscriber,
        CT_filereader,
        CT_filewriter,
        CT_udpreader
    };

    ComponentView(ComponentType ct = CT_algorithm, QGraphicsItem* parent = 0);
    ~ComponentView();

    QRectF boundingRect() const;
    QPainterPath shape() const;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    void setSource(LinkView* l);
    void setDrain(LinkView* l);

    QVariant itemChange(GraphicsItemChange change, const QVariant& value);

    QPoint sourcePoint(const LinkView* l) { return QPoint(0, boxh / 2); }
    QPoint drainPoint(const LinkView* l) { return QPoint(portw + boxw + portw, boxh / 2); }

    void updateLinks();

    enum { Type = UserType + 1 };
    int type() const { return Type; }

private:
    ComponentType subtype;

    QString algname;
    bool inputConnected;
    bool outputConnected;

    // window garnishings
    //    const static QPainterPath inputPort;
    //    const static QPainterPath outputPort;
    //    const static QPatinerPath inputZigzag;
    //    const static QPainterPath outputZigzag;
    //    const static QPainterPath antenna;

    // box dimensions
    static const int boxh = 100;
    static const int boxw = 162;
    // port dimensions
    static const int porth = 26;
    static const int portw = 22;
    // other dimensions
    static const int shadowSize = 2;
    static const int shadowOffset = (shadowSize + 2) / 2;
    static const int margin = 5;
    static const int lineHeight = 20;
    // Machine *machine;

    // Maintain an exclusive link to the previous person up the chain.
    LinkView* chainup;
    // Maintain an exclusive link to the next person down the chain.
    LinkView* chaindown;
};

/** \file
 */

#endif
