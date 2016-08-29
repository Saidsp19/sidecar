#ifndef SIDECAR_GUI_SPECTRUM_WEIGHTWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_WEIGHTWINDOW_H

#include <vector>

#include "QtCore/QObject"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace Spectrum {

class FFTSettings;

class WeightWindow : public QObject
{
    Q_OBJECT
public:

    enum Type {
	kRectangular,
	kTriangular,
	kHanning,
	kHamming,
	kBlackman,
	kNumTypes
    };

    /** Log device to use for WeightWindow log messages.

        \return Log device
    */
    static Logger::Log& Log();

    static const char* GetName(int index);

    WeightWindow(FFTSettings* fftSettings);

    void setSize(int size);

    size_t getSize() const { return window_.size(); }

    double operator[](size_t index) const { return window_[index]; }

private slots:

    void setType(int type);

private:
    std::vector<double> window_;
    int type_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
