#ifndef SIDECAR_GUI_SPECTRUM_WORKREQUEST_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_WORKREQUEST_H

#include "QtCore/QObject"

#include <fftw3.h>

#include "Messages/Video.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Spectrum {

class WeightWindow;

/** Representation of an FFT processing step. Accumulates input data that will be used for an FFT, as well as
    the results of an FFT calculation.
*/
class WorkRequest : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    /** Log device to use for WorkRequest log messages.

        \return Log device
    */
    static Logger::Log& Log();

    /** Constructor for the first or 'master' WorkRequest object. This object will create a FFTW3 plan for use
        by all other WorkRequest objects to efficiently perform their FFT calculations.

        \param weightWindow the WeightWindow object to use for input sample
        shaping. Also defines the FFT size.

        \param gateStart the first sample in the input data to use for the FFT

        \param smoothing number of input messages to accumulate and average
        before submitting to an FFT.
    */
    WorkRequest(const WeightWindow& weightWindow);

    /** Constructor for 'slave' WorkRequest objects (all but the first).

        \param master defintion to copy
    */
    WorkRequest(const WorkRequest& master);

    /** Destructor. If the master WorkRequest, destroys the FFTW3 plan object.
     */
    ~WorkRequest();

    /** Reset the counter of accumulated input messages.
     */
    void clear() { accumulated_ = 0; }

    int getAccumulated() const { return accumulated_; }

    bool addData(const Messages::Video::Ref& msg);

    size_t getFFTSize() const { return fftSize_; }

    void execute();

    const fftw_complex* getOutput() const { return output_; }

    const Messages::Video::Ref& getLastMessage() const { return last_; }

public slots:

    void setGateStart(int value);

    void setInputSmoothing(int value);

    void setZeroPad(bool value) { zeroPad_ = value; }

private:
    void initialize();

    const WeightWindow& weightWindow_;
    Messages::Video::Ref last_;
    int gateStart_;
    int inputSmoothing_;
    int accumulated_;
    size_t fftSize_;
    fftw_complex* input_;
    fftw_complex* output_;
    fftw_plan plan_;
    bool master_;
    bool zeroPad_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
