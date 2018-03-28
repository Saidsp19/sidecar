#include "QtGui/QPainter"
#include "QtGui/QPixmap"
#include "QtSvg"

#include "SvgIconMaker.h"

using namespace SideCar::GUI;

struct SvgIconMaker::Private {
    QPixmap render(const QString& name)
    {
        if (!renderer.load(name)) return QPixmap();
        QPixmap pixmap(renderer.defaultSize());
        painter.begin(&pixmap);
        renderer.render(&painter);
        painter.end();
        return pixmap;
    }

    QSvgRenderer renderer;
    QPainter painter;
};

SvgIconMaker::SvgIconMaker(QObject* parent) : QObject(parent), private_(new Private)
{
    ;
}

SvgIconMaker::~SvgIconMaker()
{
    delete private_;
}

QIcon
SvgIconMaker::make(const QString& name)
{
    QIcon icon;
    QPixmap pixmap = private_->render(QString(":/%1On.svg").arg(name));
    if (!pixmap.isNull()) icon.addPixmap(pixmap, QIcon::Normal, QIcon::On);

    pixmap = private_->render(QString(":/%1Off.svg").arg(name));
    if (!pixmap.isNull()) icon.addPixmap(pixmap, QIcon::Normal, QIcon::Off);

    return icon;
}

QIcon
SvgIconMaker::make(QChar symbol)
{
    QIcon icon;
    QFont font("Arial", 28, QFont::Bold);
    QPainter painter;

    {
        QPixmap pixmap(32, 32);
        pixmap.fill(QColor("#000000"));
        painter.begin(&pixmap);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setFont(font);
        painter.setPen(QColor("#E0E0E0"));
        painter.drawText(0, 0, 32, 32, Qt::AlignHCenter | Qt::AlignVCenter, symbol, 0);
        painter.end();
        icon.addPixmap(pixmap, QIcon::Normal, QIcon::Off);
    }

    {
        QPixmap pixmap(32, 32);
        pixmap.fill("#000000");
        painter.begin(&pixmap);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setFont(font);
        painter.setPen(QColor("#33FF33"));
        painter.drawText(0, 0, 32, 32, Qt::AlignHCenter | Qt::AlignVCenter, symbol, 0);
        painter.end();
        icon.addPixmap(pixmap, QIcon::Normal, QIcon::On);
    }

    return icon;
}
