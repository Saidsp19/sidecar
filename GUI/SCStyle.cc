#include "QtGui/QPalette"

#include "SCStyle.h"

using namespace SideCar::GUI;

SCStyle::SCStyle()
    : Super()
{
    ;
}

void
SCStyle::polish(QPalette& palette)
{
    Super::polish(palette);
    palette.setBrush(QPalette::Base, QColor("#E0E0E0"));
    return;
}
