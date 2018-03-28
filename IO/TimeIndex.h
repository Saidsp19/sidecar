#ifndef SIDECAR_IO_TIMEINDEX_H // -*- C++ -*-
#define SIDECAR_IO_TIMEINDEX_H

#include "boost/scoped_ptr.hpp"
#include <string>

namespace Logger {
class Log;
}
namespace Utils {
class FilePath;
}

namespace SideCar {
namespace IO {

class TimeIndex {
public:
    struct Entry {
        uint32_t when_;
        off_t position_;
        Entry() : when_(), position_() {}
        Entry(uint32_t when) : when_(when), position_(0) {}
        bool operator<(const Entry& rhs) const { return when_ < rhs.when_; }
    };

    using const_iterator = const Entry*;

    static Logger::Log& Log();

    static std::string const& GetIndexFileSuffix();

    /** Constructor.

        \param path location of index file to load
    */
    TimeIndex(const std::string& path);

    ~TimeIndex();

    /** Find the record with the greatest time value that is equal to or less than the given value. If the time
        is less than the first entry, the offset to the first entry is returned (0).

        \param when time to look for

        \return offset found
    */
    off_t findOnOrBefore(uint32_t when) const;

    /** Obtain the offset for the last entry.

        \return offset found
    */
    off_t getLastEntry() const { return array_[size_ - 1].position_; }

    bool empty() const { return size_ == 0; }

    size_t size() const { return size_; }

    const_iterator begin() const { return array_; }

    const_iterator end() const { return array_ + size_; }

private:
    /** Load an index file.

        \param path location of the index file
    */
    void load(const Utils::FilePath& path);

    Entry* array_;
    size_t size_;

    static std::string const kIndexFileSuffix_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
