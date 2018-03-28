#ifndef SIDECAR_GUI_ASCOPE_PEAKBAR_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_PEAKBAR_H

#include "QtCore/QList"

#include "Messages/Video.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace AScope {

/** Model of a maximum value "peak bar". Holds the largest value seen over a certain time period (set with the
    SetLifeTime() class method). After the time period has expired, the held value resets to the last seen
    value, and a new time period begins.
*/
class PeakBar {
public:
    static Logger::Log& Log();

    /** Set the lifetime of each PeakBar. This is the number of setMaxValue() calls before the PeakBar will
        relinquish a held max value and pick a new one.

        \param lifeTime
    */
    static void SetLifeTime(int lifeTime);

    /** Obtain the opacity value for a given age. Decay values range from 1.0 for new peak bar values, and ~0.3
        for those at near the end of their lifetime.

        \param age the age to convert

        \return opacity value
    */
    static double GetDecay(int age);

    /** Constructor. Initializes a new peak bar instance.
     */
    PeakBar() : age_(0) {}

    /** Scan the values found within two iterators and acquire the maximum.

        \param pos first gate to check

        \param end last gate to check

        \return true if value changed
    */
    bool update(Messages::Video::const_iterator pos, Messages::Video::const_iterator end, bool reset);

    /** Obtain the current peak value.

        \return peak value
    */
    int getValue() const { return value_; }

    /** Obtain the age of the peak value. This is the number of update() calls seen by the object without a
        change in the bar's value.

        \return age
    */
    int getAge() const { return age_; }

    /** Obtain the opacity value that corresponds to the bar's current age.

        \return
    */
    double getDecay() const { return GetDecay(age_); }

private:
    int value_;
    int age_;

    static int lifeTime_;
    static QList<double> decayLookup_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

#endif
