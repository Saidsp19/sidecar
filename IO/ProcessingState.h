#ifndef SIDECAR_IO_PROCESSINGSTATE_H // -*- C++ -*-
#define SIDECAR_IO_PROCESSINGSTATE_H

#include <string>
#include <vector>

namespace SideCar {
namespace IO {

/** Representation of a processing state in the SideCar system. Defines a Value qenumeration which defines the
    valid integral values for a processing state. Also provides class methods for converting the enumerated
    value to/from a string.
*/
class ProcessingState {
public:
    enum Value {
        kInvalid = 0,
        kInitialize,     ///< Task initialized to known state
        kAutoDiagnostic, ///< Task performing auto-diagnostics
        kCalibrate,      ///< Task doing calibration processing
        kRun,            ///< Task doing normal data processing
        kStop,           ///< Task stopped
        kFailure,        ///< Task has detected a serious problem
        kNumStates,
    };

    static const char* const* GetNames();

    /** Class method that obtains the textual name of a given state constant.

        \param state value to look for

        \return text value
    */
    static const char* GetName(Value state);

    /** Class method that attempts to locate the state index for a given name. Ignores case when searching.

        \param name the state name to search for

        \return state index if found, or kInvalid
    */
    static Value GetValue(std::string name);

    /** Determine if the given state index is a non-failure state.

        \param state the state to test

        \return true if kInitialize, kAutoDiagnostic, kCalibrate, kRun, or kStop
    */
    static bool IsNormal(ProcessingState::Value state) { return state >= kInitialize && state <= kStop; }

    /** Determine if the given state index is a active processing state

        \param state the state to test

        \return true if kAutoDiagnostic, kCalibrate, or kRun
    */
    static bool IsActive(ProcessingState::Value state) { return state >= kAutoDiagnostic && state <= kRun; }

private:
    /** Collection of display names for the states.
     */
    static const char* const displayNames_[kNumStates];

    /** Lower-case versions of the display names. Used by GetValue() to locate state index without regard for
        name case.
    */
    static std::vector<std::string> searchNames_;

    /** Internal class used to initialize the searchNames_ attribute above. The Initializer constructor copies
        lower-case versions of the values from displayNames_ to the searchNames_ vector.
    */
    class Initializer;

    /** Initialize for the searchNames_ class attribute above.
     */
    static Initializer initializer_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
