#include <algorithm> // for transform
#include <cstring>   // for toupper
#include <sstream>

#include "Logger/Priority.h"

using namespace Logger;

const char* Priority::shortNames_[Priority::kNumLevels] = {"-", "F", "E", "W", "I", "TI", "TO", "D1", "D2", "D3"};

const char* Priority::longNames_[Priority::kNumLevels] = {"NONE",     "FATAL",     "ERROR",   "WARNING", "INFO",
                                                          "TRACE-IN", "TRACE-OUT", "DEBUG-1", "DEBUG-2", "DEBUG-3"};

Priority::InvalidName::InvalidName(const std::string& name) :
    Priority::PriorityException<InvalidName>("Find", "invalid Priority name - ")
{
    *this << "'" << name << "'";
}

Priority::InvalidLevel::InvalidLevel(Priority::Level level) :
    Priority::PriorityException<InvalidLevel>("Name", "invalid Priority level - ")
{
    *this << level;
}

Priority::Level
Priority::Find(std::string name)
{
    // Convert request to uppercase before searching.
    //
    std::transform(name.begin(), name.end(), name.begin(), toupper);
    for (int index = 0; index < kNumLevels; ++index) {
        if (name == shortNames_[index] || name == longNames_[index]) { return Level(index); }
    }

    // Try and convert to an integer
    //
    std::istringstream is(name);
    int index;
    is >> index;
    if (!is || index < kNone || index > kDebug3) throw InvalidName(name);
    return Level(index);
}

const char*
Priority::GetShortName(Level level)
{
    if (level < kNone || level > kDebug3) throw InvalidLevel(level);
    return shortNames_[level];
}

const char*
Priority::GetLongName(Level level)
{
    if (level < kNone || level > kDebug3) throw InvalidLevel(level);
    return longNames_[level];
}

const char* const*
Priority::GetShortNames()
{
    return shortNames_;
}

const char* const*
Priority::GetLongNames()
{
    return longNames_;
}
