#ifndef SIDECAR_GUI_SCSTYLE_H // -*- C++ -*-
#define SIDECAR_GUI_SCSTYLE_H

#include "QtWidgets/QProxyStyle"

class QPainterPath;

namespace SideCar {
namespace GUI {

class SCStyle : public QProxyStyle {
    Q_OBJECT
    using Super = QProxyStyle;

public:
    SCStyle();

    void polish(QPalette& palette);
};

} // namespace GUI
} // namespace SideCar

#endif
