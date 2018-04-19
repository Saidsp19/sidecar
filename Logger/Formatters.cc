// -*- C++ -*-

#include <iomanip>
#include <iostream>

#include "Formatters.h"
#include "Msg.h"

using namespace Logger;
using namespace Logger::Formatters;

static std::ostream&
operator<<(std::ostream& os, const timeval& when)
{
    time_t secs = when.tv_sec;
    int subs = static_cast<int>(when.tv_usec * 1.0E-4);
    struct tm* bits = ::gmtime(&secs);
    char buffer[128];
    ::strftime(buffer, 128, "%Y%m%d %H%M%S", bits);
    return os << buffer << '.' << std::setfill('0') << std::setw(2) << subs << std::setfill(' ');
}

void
Terse::format(std::ostream& os, const Msg& msg)
{
    const std::string& body = msg.message_;
    os << msg.when_ << ' ' << Priority::GetShortName(msg.level_) << " - " << body;
}

void
Verbose::format(std::ostream& os, const Msg& msg)
{
    const std::string& body = msg.message_;
    os << msg.when_ << ' ' << Priority::GetLongName(msg.level_) << ' ' << msg.channel_ << ':';
    if (!body.empty() && body[0] != ':') os << ' ';
    os << body;
}

void
Pattern::format(std::ostream& os, const Msg& msg)
{
    std::string::const_iterator pos(pattern_.begin());
    std::string::const_iterator end(pattern_.end());
    time_t seconds;
    char buf[128];
    std::string timeFormat;
    struct tm* bits;

    // Walk the formatting string, from start to finish
    //
    while (pos != end) {
        char c = *pos++;
        if (c != '%') {
            os << c;
        } else {
            // Error if it is at the end of the format string.
            //
            if (pos == end) throw std::out_of_range(pattern_);

            // Interrogate the following character.
            //
            switch ((c = *pos++)) {
                // Emit '%'
                //
            case '%':
                os << c;
                break;

                // Emit timestamp in TimeVal format
                //
            case 'w':
                os << msg.when_;
                break;

                // Emit timestamp in Unix `date' format.
                //
            case 'W':
                seconds = msg.when_.tv_sec;
                bits = ::localtime(&seconds);
                // Wed Dec 31 19:02:03 EST 1969
                ::strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Z %Y", bits);
                os << buf;
                break;

                // Emit timestamp in custom format, ala strftime.
                //
            case '\'':

                // Built format string for strformat by scarfing characters until we encounter the closing single
                // quote.
                //
                timeFormat = "";
                while (pos != end && *pos != '\'') { timeFormat += *pos++; }

                // We expect a closing single quote.
                //
                if (pos == end) throw std::out_of_range(pattern_);
                ++pos;

                // Use format string and write to output stream.
                //
                seconds = msg.when_.tv_sec;
                ::strftime(buf, sizeof(buf), timeFormat.c_str(), ::localtime(&seconds));
                os << buf;
                break;

                // Emit timestamp seconds value
                //
            case 'S':
                os << msg.when_.tv_sec;
                break;

                // Emit timestamp microseconds values
                //
            case 'M':
                os << msg.when_.tv_usec;
                break;

                // Emit msg channel
                //
            case 'c':
                os << msg.channel_;
                break;

                // Emit msg priority numeric value
                //
            case 'l':
                os << msg.level_;
                break;

                // Emit msg priority name
                //
            case 'p':
                os << Priority::GetShortName(msg.level_);
                break;

                // Emit msg priority value
                //
            case 'P':
                os << Priority::GetLongName(msg.level_);
                break;

                // Emit msg message value
                //
            case 'm':
                os << msg.message_;
                break;

                // Emit linefeed
                //
            case 'z':
                os << '\n';
                break;

                // Unknown format, so just emit literal characters.
                //
            default: os << '%' << c; break;
            }
        }
    }
}
