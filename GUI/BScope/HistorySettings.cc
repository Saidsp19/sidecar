#include "HistorySettings.h"

using namespace SideCar::GUI::BScope;

HistorySettings::HistorySettings(IntSetting* frameCount, BoolSetting* frameHasGrid, BoolSetting* frameHasRangeMap,
                                 BoolSetting* frameHasExtractions, BoolSetting* frameHasRangeTruths,
                                 BoolSetting* frameHasBugPlots) :
    Super(),
    frameCount_(frameCount), frameHasGrid_(frameHasGrid), frameHasRangeMap_(frameHasRangeMap),
    frameHasExtractions_(frameHasExtractions), frameHasRangeTruths_(frameHasRangeTruths),
    frameHasBugPlots_(frameHasBugPlots)
{
    add(frameCount);
    add(frameHasGrid);
    add(frameHasRangeMap);
    add(frameHasExtractions);
    add(frameHasRangeTruths);
    add(frameHasBugPlots);
    connect(frameCount, SIGNAL(valueChanged(int)), SIGNAL(frameCountChanged(int)));
}
