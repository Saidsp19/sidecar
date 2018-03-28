#ifndef SIDECAR_IO_RECORDINDEX_H // -*- C++ -*-
#define SIDECAR_IO_RECORDINDEX_H

#include <string>

namespace Logger {
class Log;
}
namespace Utils {
class FilePath;
}

namespace SideCar {
namespace Messages {
class MetaTypeInfo;
}
namespace IO {

class RecordIndex {
public:
    using const_iterator = const off_t*;

    static Logger::Log& Log();

    static std::string const& GetIndexFileSuffix();

    /** Constructor.

        \param path location of index file to use
    */
    RecordIndex(const std::string& path);

    ~RecordIndex();

    off_t operator[](size_t index) const { return array_[index]; }

    bool empty() const { return size_ == 0; }

    size_t size() const { return size_; }

    const_iterator begin() const { return array_; }

    const_iterator end() const { return array_ + size_; }

private:
    /** Load an index file.

        \param path location of the index file
    */
    void load(const Utils::FilePath& path);

    off_t* array_;
    size_t size_;
    const Messages::MetaTypeInfo* metaTypeInfo_;

    static std::string const kIndexFileSuffix_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
