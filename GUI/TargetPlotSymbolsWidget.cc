#include "TargetPlotSymbolsWidget.h"
#include "TargetPlotImaging.h"

#include "ui_TargetPlotSymbolsWidget.h"

using namespace SideCar::GUI;

TargetPlotSymbolsWidget::TargetPlotSymbolsWidget(QWidget* parent)
    : Super(parent), gui_(new Ui::TargetPlotSymbolsWidget)
{
    gui_->setupUi(this);
}

void
TargetPlotSymbolsWidget::connectExtractionsSymbolType(TargetPlotImaging* setting)
{
    gui_->extractions_->associateWith(setting);
}

void
TargetPlotSymbolsWidget::connectRangeTruthsSymbolType(TargetPlotImaging* setting)
{
    gui_->rangeTruths_->associateWith(setting);
}

void
TargetPlotSymbolsWidget::connectBugPlotsSymbolType(TargetPlotImaging* setting)
{
    gui_->bugPlots_->associateWith(setting);
}
