#ifndef SIDECAR_GUI_SCSTYLE_H // -*- C++ -*-
#define SIDECAR_GUI_SCSTYLE_H

#include "QtGui/QCleanlooksStyle"

class QPainterPath;

namespace SideCar {
namespace GUI {

class SCStyle : public QCleanlooksStyle {
    Q_OBJECT
    using Super = QCleanlooksStyle;

public:
    SCStyle();

    void polish(QPalette& palette);
};

} // namespace GUI
} // namespace SideCar

#endif
