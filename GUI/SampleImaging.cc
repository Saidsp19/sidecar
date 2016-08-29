#include "SampleImaging.h"
#include "Setting.h"

using namespace SideCar::GUI;

SampleImaging::SampleImaging(BoolSetting* enabled, ColorButtonSetting* color,
                             DoubleSetting* pointSize, OpacitySetting* alpha,
                             QComboBoxSetting* decimation)
    : Super(enabled, color, pointSize, alpha), decimation_(decimation)
{
    connect(decimation, SIGNAL(valueChanged(int)), this,
            SIGNAL(decimationChanged(int)));
}
