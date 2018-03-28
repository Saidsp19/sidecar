#include "GUI/LogUtils.h"
#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "FFTSettings.h"
#include "WeightWindow.h"

using namespace SideCar::GUI::Spectrum;

static const char* kWindowNames[] = {"Rectangle", "Triangle", "Hanning", "Hamming", "Blackman"};

Logger::Log&
WeightWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("spectrum.WeightWindow");
    return log_;
}

const char*
WeightWindow::GetName(int index)
{
    return kWindowNames[index];
}

WeightWindow::WeightWindow(FFTSettings* fftSettings) : QObject(), window_(), type_()
{
    window_.resize(fftSettings->getFFTSize(), 0.0);
    setType(fftSettings->getWindowType());
    connect(fftSettings, SIGNAL(windowTypeChanged(int)), SLOT(setType(int)));
}

void
WeightWindow::setSize(int size)
{
    window_.resize(size);
    setType(type_);
}

void
WeightWindow::setType(int type)
{
    Logger::ProcLog log("setType", Log());
    LOGINFO << "type: " << type << std::endl;

    type_ = type;

    size_t size = window_.size();
    size_t index = 0;

    switch (type) {
    case kRectangular:
        for (; index < size; ++index) window_[index] = 1.0;
        break;

    case kTriangular:
        for (; index < size / 2; ++index) window_[index] = 2.0 * index / size;
        for (; index < size; ++index) window_[index] = 2.0 - 2.0 * index / size;
        break;

    case kHanning:
        for (; index < size; ++index) window_[index] = 0.5 - 0.5 * ::cos(Utils::kCircleRadians * index / size);
        break;

    case kHamming:
        for (; index < size; ++index) window_[index] = 0.54 - 0.46 * ::cos(Utils::kCircleRadians * index / size);
        break;

    case kBlackman:
        for (; index < size; ++index)
            window_[index] = 0.42 - 0.50 * ::cos(Utils::kCircleRadians * index / size) +
                             0.08 * ::cos(2 * Utils::kCircleRadians * index / size);
        break;

    default: ::abort(); break;
    }
}
