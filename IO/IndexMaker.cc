#include "Logger/Log.h"
#include "Messages/Header.h"
#include "Messages/Video.h"
#include "Time/TimeStamp.h"
#include "Utils/Exception.h"
#include "Utils/Utils.h"

#include "AutoCloseFileDescriptor.h"
#include "Decoder.h"
#include "IndexMaker.h"
#include "Readers.h"
#include "RecordIndex.h"
#include "TimeIndex.h"
using namespace SideCar::IO;

Logger::Log&
IndexMaker::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.IndexMaker");
    return log_;
}

IndexMaker::Status
IndexMaker::Make(const std::string& inputFilePath, uint16_t rate, StatusProc proc)
{
    Logger::ProcLog log("Make", Log());
    LOGINFO << inputFilePath << ' ' << rate << ' ' << proc << std::endl;

    AutoCloseFileDescriptor ifd(::open(inputFilePath.c_str(), O_RDONLY));
    if (! ifd) {
	LOGERROR << "failed to open data file " << inputFilePath << " - " << errno << ' ' << strerror(errno)
		 << std::endl;
	return kFileOpenFailed;
    }

    IO::FileReader reader;
    reader.getDevice().set_handle(ifd);

    // Get the last file position of the file so we can calculate percentage complete.
    //
    reader.getDevice().seek(0, SEEK_END);
    off_t endPos = reader.getDevice().tell();
    reader.getDevice().seek(0, SEEK_SET);
    double posRatio = 1.0 / static_cast<double>(endPos);

    Utils::FilePath fp(inputFilePath);
    fp.setExtension(RecordIndex::GetIndexFileSuffix());
    ::unlink(fp.c_str());
    int openFlags = O_WRONLY | O_CREAT | O_TRUNC;
    mode_t permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    AutoCloseFileDescriptor rfd(::open(fp.c_str(), openFlags, permissions));
    if (! rfd) {
	LOGERROR << "failed to create record index file " << fp << " - "
		 << errno << ' ' << strerror(errno) << std::endl;
	return kRecordIndexCreateFailed;
    }

    fp.setExtension(TimeIndex::GetIndexFileSuffix());
    ::unlink(fp.c_str());
    AutoCloseFileDescriptor tfd(::open(fp.c_str(), openFlags, permissions));
    if (! tfd) {
	LOGERROR << "failed to create time index file " << fp << " - " << errno << ' ' << strerror(errno)
                 << std::endl;
	return kTimeIndexCreateFailed;
    }

    Messages::Header header(Messages::Video::GetMetaTypeInfo());
    TimeIndex::Entry entry(0);
    size_t inputCounter = 0;
    size_t recordIndexCounter = 0;
    size_t timeIndexCounter = 0;

    Time::TimeStamp start = Time::TimeStamp::Now();
    while (1) {

	// Remember the current file position
	//
	off_t pos = reader.getDevice().tell();

	// Fetch the next message record from the input file.
	//
	if (! reader.fetchInput()) {

	    // If the input file contains more than one record, add a final entry to the TimeIndex file which
	    // points to the last entry in the input file. This will hold the emitted timestamp of the last
	    // entry.
	    //
	    if (inputCounter > 1) {

		// Update with the last record in the file.
		//
		entry.when_ = header.getEmittedTimeStamp().getSeconds();
		int rc = ::write(tfd, &entry, sizeof(entry));
		if (rc == -1) {
		    LOGERROR << "failed ::write() - " << errno << ' ' << strerror(errno) << std::endl;
		    return kTimeIndexWriteFailed;
		}
		++timeIndexCounter;
	    }
	    break;
	}

	if (! reader.isMessageAvailable()) {
	    LOGERROR << "isMessageAvailable returned false" << std::endl;
	    return kDataReadFailed;
	}

	entry.position_ = pos;
	++inputCounter;

	int rc = ::write(rfd, &entry.position_, sizeof(entry.position_));
	if (rc == -1) {
	    LOGERROR << "failed ::write() - " << errno << ' ' << strerror(errno) << std::endl;
	    return kRecordIndexWriteFailed;
	}

	++recordIndexCounter;

	// Decode just the header so we can get to the time stamp.
	//
	IO::Decoder decoder(reader.getMessage());
	header.load(decoder);

	// If this is the first record or the amount of time that has elapsed is >= sampleRate then write a new
	// Entry record
	//
	uint32_t delta = header.getEmittedTimeStamp().getSeconds();
	delta -= entry.when_;
	if (entry.position_ == 0 || delta >= rate) {
	    entry.when_ = header.getEmittedTimeStamp().getSeconds();
	    rc = ::write(tfd, &entry, sizeof(entry));
	    if (rc == -1) {
		LOGERROR << "failed ::write() - " << errno << ' ' << strerror(errno) << std::endl;
		return kTimeIndexWriteFailed;
	    }
	    ++timeIndexCounter;
	}

	if (proc) {
	    Time::TimeStamp elapsed(Time::TimeStamp::Now());
	    elapsed -= start;
	    if (elapsed.getSeconds() > 1) {
		start = Time::TimeStamp::Now();
		LOGDEBUG << "calling proc" << std::endl;
		proc(pos * posRatio);
	    }
	}
    }

    LOGDEBUG << "read " << inputCounter << " messages" << std::endl;
    LOGDEBUG << "wrote " << recordIndexCounter << " position records" << std::endl;
    LOGDEBUG << "wrote " << timeIndexCounter << " time records" << std::endl;

    return kOK;
}


