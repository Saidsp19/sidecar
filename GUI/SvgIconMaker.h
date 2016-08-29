#ifndef SIDECAR_GUI_SVGICONMAKER_H // -*- C++ -*-
#define SIDECAR_GUI_SVGICONMAKER_H

#include "QtCore/QObject"
#include "QtGui/QIcon"

namespace SideCar {
namespace GUI {

class SvgIconMaker : public QObject
{
    Q_OBJECT
public:

    SvgIconMaker(QObject* parent = 0);

    ~SvgIconMaker();

    QIcon make(const QString& name);

    QIcon make(QChar symbol);

private:
    struct Private;
    Private* private_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
