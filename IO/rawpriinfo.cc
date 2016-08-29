#include <cmath>
#include <iostream>
#include <iomanip>
#include <string>

#include "ace/FILE_Connector.h"
#include "boost/algorithm/minmax.hpp"
#include "boost/algorithm/minmax_element.hpp"

#include "IO/FileReaderTask.h"
#include "IO/MessageManager.h"
#include "Messages/Video.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/Utils.h"

using namespace SideCar;

const std::string about = "Load and print information about a raw PRI file.";

const Utils::CmdLineArgs::OptionDef opts[] = {
    { 'd', "data", "print PRI data for each record", 0 },
    { 'h', "header", "print PRI header for each record", 0 },
};

const Utils::CmdLineArgs::ArgumentDef args[] = {
    { "FILE", "path to input file" }
};

int
main(int argc, char** argv)
{
    Utils::CmdLineArgs cla(argc, argv, about, opts, sizeof(opts),
                           args, sizeof(args));
    bool showHeader = cla.hasOpt("header");
    bool showData = cla.hasOpt("data");
    if (! showData && ! showHeader) showHeader = true;

    IO::FileReader::Ref reader(IO::FileReader::Make());
    ACE_FILE_Addr filePath(cla.arg(00).c_str());
    ACE_FILE_Connector connector;
    if (connector.connect(reader->getDevice(), filePath, 0, ACE_Addr::sap_any,
                          0, O_RDONLY, ACE_DEFAULT_FILE_PERMS) == -1) {
	std::cerr << "failed to open file " << cla.arg(0) << std::endl;
	return 1;
    }

    std::vector<int16_t> samples;
    int fd = reader->getDevice().get_handle();
    off_t recordPosition = ::lseek(fd, 0, SEEK_SET);
    if (showHeader) {
	std::cout << std::setw(10) << "Offset" << ' '
		  << std::setw(10) << "Sequence" << ' '
		  << std::setw(5) << "Shaft" << ' '
		  << std::setw(5) << "Count" << '\n';
	std::cout << std::setw(10) << "------" << ' '
		  << std::setw(10) << "--------" << ' '
		  << std::setw(5) << "-----" << ' '
		  << std::setw(5) << "-----" << '\n';
    }

    while (reader->fetchInput()) {
	if (reader->isMessageAvailable()) {

	    ACE_Message_Block* msg = reader->getMessage();
	    ACE_InputCDR cdr(msg);

	    uint16_t magic;
	    cdr >> magic;
	    if (magic != 0xAAAA) {
		std::cerr << "invalid magic word - " << magic << std::endl;
		return 1;
	    }

	    uint16_t byteOrder;
	    cdr >> byteOrder;
	    if (byteOrder != 0 && byteOrder != 0xFFFF) {
		std::cerr << "invalid byte order - " << byteOrder << std::endl;
		return 1;
	    }

	    cdr.reset_byte_order(byteOrder ? 1 : 0);
	    uint32_t size;
	    cdr >> size;

	    uint16_t headerVersion;
	    cdr >> headerVersion;
	    if (headerVersion != 1) {
		std::cerr << "invalid header version - " << headerVersion
			  << std::endl;
		return 1;
	    }

	    uint16_t guidVersion;
	    cdr >> guidVersion;
	    if (guidVersion != 2) {
		std::cerr << "invalid GUID version - " << guidVersion
			  << std::endl;
		return 1;
	    }

	    std::string producer;
	    cdr >> producer;
	    if (producer != "RawPRI") {
		std::cerr << "invalid producer tag - " << producer << std::endl;
		return 1;
	    }

	    uint16_t messageTypeKey;
	    cdr >> messageTypeKey;
	    if (messageTypeKey != 2) {
		std::cerr << "invalid message type key - " << messageTypeKey
			  << std::endl;
		return 1;
	    }

	    uint32_t messageSequenceNumber;
	    cdr >> messageSequenceNumber;

	    std::string representation;
	    cdr >> representation;
	    if (representation.size() != 0) {
		std::cerr << "invalid GUID representation - " << representation
			  << std::endl;
		return 1;
	    }

	    int32_t seconds, microSeconds;
	    cdr >> seconds;
	    cdr >> microSeconds;

	    double gap1;
	    cdr >> gap1;

	    double rangeMin;
	    cdr >> rangeMin;
	    if (rangeMin != 0.0) {
		std::cerr << "invalid rangeMin - " << rangeMin << std::endl;
		return 1;
	    }

	    double rangeFactor;
	    cdr >> rangeFactor;
	    if (rangeFactor <= 0.0) {
		std::cerr << "invalid rangeFactor - " << rangeFactor
			  << std::endl;
		return 1;
	    }

	    double beamWidth;
	    cdr >> beamWidth;
	    if (beamWidth <= 0.0) {
		std::cerr << "invalid beamWidth - " << beamWidth << std::endl;
		return 1;
	    }

	    uint16_t shaftEncoding;
	    uint16_t shaftEncodingRange;
	    cdr >> shaftEncoding;
	    cdr >> shaftEncodingRange;
	    if (shaftEncodingRange != 0x1000) {
		std::cerr << "invalid shaftEncodingRange - "
			  << shaftEncodingRange << std::endl;
		return 1;
	    }
	    if (shaftEncoding >= shaftEncodingRange) {
		std::cerr << "invalid shaftEncoding - " << shaftEncoding
			  << std::endl;
		return 1;
	    }

	    uint16_t northMark;
	    cdr >> northMark;
	    if (northMark >= shaftEncodingRange) {
		std::cerr << "invalid northMark - " << northMark << std::endl;
		return 1;
	    }

	    uint32_t sampleCount;
	    cdr >> sampleCount;
	    if (sampleCount == 0) {
		std::cerr << "invalid sampleCount - " << sampleCount
			  << std::endl;
		return 1;
	    }

	    samples.resize(sampleCount);

	    if (! cdr.read_short_array(&samples[0], sampleCount)) {
		std::cerr << "failed cdr.read_short_array" << std::endl;
		return 1;
	    }

	    if (cdr.length() > 0) {
		std::cerr << "unprocessed bytes in stream - " << cdr.length()
			  << std::endl;
		return 1;
	    }

	    if (showHeader) {
		std::cout << std::setw(10) << recordPosition << ' '
			  << std::setw(10) << messageSequenceNumber << ' '
			  << std::setw(5) << shaftEncoding << ' '
			  << std::setw(5) << sampleCount << '\n';
	    }
	    else if (showData) {
		std::cout << "*** Offset: " << recordPosition
			  << " Sequence: " << messageSequenceNumber
			  << " Shaft: " << shaftEncoding
			  << " Count: " << sampleCount << '\n';
	    }

	    if (showData) {
		std::cout << '\n';
		short* ptr = &samples[0];
		while (sampleCount) {
		    for (int index = 0; index < 13 && sampleCount;
                         ++index, --sampleCount)
			std::cout << std::setw(5) << *ptr++ << ' ';
		    std::cout << '\n';
		}

		std::cout << '\n';
	    }

	    recordPosition = ::lseek(fd, 0, SEEK_CUR);
	}
    }

    return 0;
}
