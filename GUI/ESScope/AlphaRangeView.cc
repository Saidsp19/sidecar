#include "AlphaRangeView.h"
#include "AlphaRangeWidget.h"

using namespace SideCar::GUI::ESScope;

AlphaRangeView::AlphaRangeView(QWidget* parent, ViewSettings* viewSettings) :
    Super(parent, viewSettings, "Alpha/Range", "Alpha", "Range")
{
    setObjectName("AlphaRangeView");
    setXYWidget(new AlphaRangeWidget(this, viewSettings));
}

AlphaRangeWidget*
AlphaRangeView::getDisplay() const
{
    return qobject_cast<AlphaRangeWidget*>(Super::getDisplay());
}
