#include "AlphaBetaView.h"
#include "AlphaBetaWidget.h"

using namespace SideCar::GUI::ESScope;

AlphaBetaView::AlphaBetaView(QWidget* parent, ViewSettings* viewSettings)
    : Super(parent, viewSettings, "Alpha/Beta", "Alpha", "Beta")
{
    setObjectName("AlphaBetaView");
    setXYWidget(new AlphaBetaWidget(this, viewSettings));
}

AlphaBetaWidget*
AlphaBetaView::getDisplay() const
{
    return qobject_cast<AlphaBetaWidget*>(Super::getDisplay());
}
