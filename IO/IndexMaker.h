#ifndef SIDECAR_IO_INDEXMAKER_H // -*- C++ -*-
#define SIDECAR_IO_INDEXMAKER_H

#include "boost/function.hpp"

#include "Utils/FilePath.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

class IndexMaker 
{
public:
    static Logger::Log& Log();

    enum Status {
	kOK = 0,
	kFileOpenFailed,
	kRecordIndexCreateFailed,
	kRecordIndexWriteFailed,
	kTimeIndexCreateFailed,
	kTimeIndexWriteFailed,
	kDataReadFailed
    };

    using StatusProc = boost::function<void (double)>;

    static Status Make(const std::string& inputFilePath, uint16_t rate, StatusProc proc = 0);
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
