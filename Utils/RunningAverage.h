#ifndef UTILS_RUNNINGAVERAGE_H // -*- C++ -*-
#define UTILS_RUNNINGAVERAGE_H

#include <cstddef>
#include <vector>

namespace Logger {
class Log;
}

namespace Utils {

/** Container class the maintains a running average value for a stream of values. Instances have a fixed window
    size that determines how many samples to track for calculations; after receiving this number of samples, the
    algorithm starts to forget the oldest sample.
*/
class RunningAverage {
public:
    /** Obtain Logger::Log device to use for instances of this class.

        \return Logger::Log reference
    */
    static Logger::Log& Log();

    /** Constructor. Creates a new container for windowSize samples, initialized to initialValue values.

        \param windowSize number of samples to remember

        \param initialValue initial value to give to window samples
    */
    RunningAverage(size_t windowSize, double initialValue = 0.0);

    /** Add a new sample to the container.

        \param value sample value to add
    */
    void addValue(double value);

    /** Obtain the current average value for the samples seen so far.

        \return current average value
    */
    double getAverageValue() const { return sum_ / values_.size(); }

    /** Reset the container so that all samples have a given initial value.

        \param initialValue the initial value to use
    */
    void clear(double initialValue = 0.0);

    /** Reset the container so that it has a new sample window size. If the given window size is the same as the
        current one, the effect will be the same as calling clear(initialValue).

        \param windowSize new size to use

        \param initialValue the initial value to use for samples
    */
    void setWindowSize(size_t windowSize, double initialValue = 0.0);

private:
    std::vector<double> values_;
    size_t oldest_;
    double sum_;
};

} // end namespace Utils

#endif
