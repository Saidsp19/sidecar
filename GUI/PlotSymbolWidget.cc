#include "PlotSymbolWidget.h"
#include "TargetPlotImaging.h"

using namespace SideCar::GUI;

void
PlotSymbolWidget::associateWith(TargetPlotImaging* settings)
{
    settings_ = settings;
    settings_->connectPlotSymbolWidget(this);
    connect(settings, SIGNAL(colorChanged()), SLOT(colorChanged()));
}

void
PlotSymbolWidget::colorChanged()
{
    settings_->addSymbolIcons(this);
}
