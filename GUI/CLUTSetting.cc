#include "CLUTSetting.h"

using namespace SideCar::GUI;

CLUTSetting::CLUTSetting(PresetManager* mgr, QComboBox* widget, bool global)
    : Super(mgr, widget, global)
{
    ;
}

void
CLUTSetting::connectWidget(QComboBox* widget)
{
    CLUT::AddTypeNames(widget);
    Super::connectWidget(widget);
}
