#ifndef LOGGER_PRIORITY_H // -*- C++ -*-
#define LOGGER_PRIORITY_H

#include "Logger/Exceptions.h"
#include <string>

namespace Logger {

/** Representation of a log message priority. All messages are issued at a certain priority level, and Log
    instances have a level setting, below which no messages are emitted.
*/
class Priority {
public:
    /** Level values.
     */
    enum Level {
        kNone = 0,
        kFatal,
        kError,
        kWarning,
        kInfo,
        kTraceIn,
        kTraceOut,
        kDebug,
        kDebug1 = kDebug,
        kDebug2,
        kDebug3,
        kNumLevels
    };

    /** Base class for all Priority class exceptions.
     */
    template <typename T>
    struct PriorityException : public LoggerException<T> {
        PriorityException(const char* routine, const char* err) : LoggerException<T>("Priority", routine)
        {
            *this << err;
        }
    };

    /** Invalid priority name given to the Find class method.
     */
    struct InvalidName : public PriorityException<InvalidName> {
        InvalidName(const std::string& name);
    };

    /** Invalid priority level given to the Find class method.
     */
    struct InvalidLevel : public PriorityException<InvalidLevel> {
        InvalidLevel(Priority::Level level);
    };

    /** Locate a Priority instance under a given name.

        \param name key to use for the search

        \return Priority instance found
    */
    static Priority::Level Find(std::string name);

    /** Locate a Priority instance with a given level.

        \param level to look for

        \return Priority instance found
    */
    static const char* GetShortName(Level level);

    /** Locate a Priority instance with a given level.

        \param level to look for

        \return Priority instance found
    */
    static const char* GetLongName(Level level);

    /** Obtain a pointer to all of the priority names.

        \return read-only C string array
    */
    static const char* const* GetShortNames();

    /** Obtain a pointer to all of the priority names.

        \return read-only C string array
    */
    static const char* const* GetLongNames();

private:
    Priority();

    static const char* shortNames_[kNumLevels];
    static const char* longNames_[kNumLevels];
};

} // end namespace Logger

/** \file
 */

#endif
