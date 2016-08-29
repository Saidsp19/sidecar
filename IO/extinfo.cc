#include <cmath>
#include <iostream>
#include <string>

#include "ace/FILE_Connector.h"
#include "boost/algorithm/minmax.hpp"
#include "boost/algorithm/minmax_element.hpp"

#include "IO/FileReaderTask.h"
#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Messages/RadarConfig.h"
#include "Messages/Extraction.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/FilePath.h"
#include "Utils/Format.h"
#include "Utils/Utils.h"

using namespace SideCar;

const std::string about = "Print information about an extraction PRI file.";

const Utils::CmdLineArgs::OptionDef opts[] = {
    { 'D', "debug", "enable root debug level", 0 },
    { 'c', "csv", "print PRI data in comma-separated form", 0 },
    { 'd', "data", "print PRI data for each record", 0 },
    { 'h', "header", "print PRI header for each record", 0 },
};

const Utils::CmdLineArgs::ArgumentDef args[] = {
    { "FILE", "path to input file" }
};

/** Simple task that places messages given to its put() method into its message queue.
 */
struct PRIInfoInputTask : public IO::Task
{
    /** Constructor.
     */
    PRIInfoInputTask() : IO::Task() {}

    /** Override of IO::Task method. Puts message into internal message queue.

        \param block message data

        \param timeout amount of time to spend trying to insert

        \return true if successful
    */
    bool deliverControlMessage(ACE_Message_Block* block,
                               ACE_Time_Value* timeout)
	{ return putq(block, timeout) != -1; }

    /** Override of IO::Task method. Puts message into internal message queue.

        \param block message data

        \param timeout amount of time to spend trying to insert

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* block, ACE_Time_Value* timeout)
	{ return putq(block, timeout) != -1; }
};

int
main(int argc, char** argv)
{
    Utils::CmdLineArgs cla(argc, argv, about, opts, sizeof(opts), args,
                           sizeof(args));
    if (cla.hasOpt("debug"))
	Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    Utils::FilePath filePath(cla.arg(0));
    if (! filePath.exists()) {
	std::cerr << "*** file '" << filePath << "' does not exists\n";
	return 1;
    }

    bool showHeader = true;
    bool printData = cla.hasOpt("data");
    bool printHeader = cla.hasOpt("header");
    bool printCSV = cla.hasOpt("csv");

    // Create a new FileReaderTask object.
    //
    IO::FileReaderTask::Ref reader(IO::FileReaderTask::Make());

    // Create a new input task to collect the messages read in from the reader task. The reader task will invoke
    // InputTask::put() for each message it reads in.
    //
    PRIInfoInputTask input;
    reader->next(&input);

    if (! reader->openAndInit("Extractions", filePath.c_str())) {
	std::cerr << "*** failed to open/start reader on file '"
		  << cla.arg(0) << "'" << std::endl;
	return 1;
    }

    reader->start();

    ACE_Message_Block* data;
    while (input.getq(data, 0) != -1) {
	IO::MessageManager mgr(data);
	if (mgr.isShutdownRequest()) break;

	Messages::Header::Ref msg = mgr.getNative();

	if (printData) {
	    if (printCSV) {

		if (showHeader) {
		    std::cout << "YYYYMMDD, HH:MM:SS.SS, "
			      << "    RANGE (km), "
			      << " AZIMUTH (rad), "
			      << "     ELEVATION, "
			      << "        X (km), "
			      << "        Y (km)\n";
		    showHeader = false;
		}

		Messages::Extractions::Ref extraction =
		    mgr.getNative<Messages::Extractions>();
		Messages::Extractions::const_iterator pos =
		    extraction->begin();
		Messages::Extractions::const_iterator end =
		    extraction->end();

		Utils::Format date(2, 2);
		date.fill('0');

		Utils::Format coord(14, 8);
		coord.fixed().showpoint();

		while (pos != end) {
		    time_t t = pos->getWhen().getSeconds();
		    tm* bits = ::localtime(&t);
		    std::cout << date(bits->tm_year + 1900)
			      << date(bits->tm_mon + 1)
			      << date(bits->tm_mday) << ", "
			      << date(bits->tm_hour) << ':'
			      << date(bits->tm_min) << ':'
			      << date(bits->tm_sec) << '.'
			      << date(int(pos->getWhen().getMicro() *
                                          Time::TimeStamp::kSecondsPerMicro *
                                          100)) << ", "
			      << coord(pos->getRange()) << ", "
			      << coord(pos->getAzimuth()) << ", "
			      << coord(pos->getElevation()) << ", "
			      << coord(pos->getX()) << ", "
			      << coord(pos->getY()) << '\n';
		    ++pos;
		}
	    }
	    else {
		std::cout << msg->dataPrinter() << '\n';
	    }
	}
	else if (printHeader) {
	    std::cout << msg->headerPrinter() << '\n';
	}
    }

    return 0;
}
