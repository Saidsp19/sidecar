#include "Logger/ConfiguratorFile.h"
#include "Logger/Log.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/Exception.h"
#include "Utils/FilePath.h"
#include "Utils/Utils.h"

#include "IndexMaker.h"
#include "RecordIndex.h"
#include "TimeIndex.h"

using namespace SideCar::IO;

const char* description = "Create a index files for a messages file.";

const Utils::CmdLineArgs::OptionDef options[] = {
    { 'L', "logger", "use CFG for Logger configuration file", "CFG" },
    { 'g', "gap", "number of seconds between time position records", "GAP" },
    { 'i', "input", "generate index files for data FILE", "FILE" },
    { 'r', "record", "read in RECORD index file", "RECORD" },
    { 't', "time", "read in TIME index file", "TIME" }
};

const Utils::CmdLineArgs::ArgumentDef args[] = {
    { 0, 0 },
    { "KEY", "search key for -r and -t options" }
};


/** Command-line argument processor. Uses Utils::CmdLineArgs to the parsing.
 */
struct Args
{
    std::string inputFile;	///< Path of file to read
    std::string recordIndex;	///< Path to RecordIndex data file
    std::string timeIndex;	///< Path to TimeIndex data file
    int gap;			///< Number of seconds between TimeIndex records
    int searchKey;		///< Time to search for if != -1

    /** Constructor. Validate command-line arguments.

        \param argc command-line argument count

        \param argv command-line argument value vector
    */
    Args(int argc, char** argv);
};

Args::Args(int argc, char** argv)
    : inputFile(""), recordIndex(""), timeIndex(""), gap(60),
      searchKey(-1)
{
    Utils::CmdLineArgs cla(argc, argv, description, options, sizeof(options),
                           args, sizeof(args));

    std::string value;
    if (cla.hasOpt("logger", value)) {
	Logger::ConfiguratorFile cf(value);
    }

    if (cla.hasOpt("gap", value)) value >> gap;

    cla.hasOpt("input", inputFile);
    cla.hasOpt("record", recordIndex);
    cla.hasOpt("time", timeIndex);

    if (cla.hasArg(0, value)) {
	value >> searchKey;
    }

    if (cla.argc()) cla.usage("too many parameters on command-line");
}

int
main(int argc, char** argv)
{
    Args args(argc, argv);
    try {
	if (args.inputFile.size()) {
	    IndexMaker::Make(args.inputFile, args.gap);
	}
	else if (args.recordIndex.size()) {
	    Utils::FilePath path(args.recordIndex);
	    path.setExtension(RecordIndex::GetIndexFileSuffix());
	    RecordIndex rix(path);
	    if (args.searchKey < 0) {
		std::cout << rix.size() << '\n';
	    }
	    else {
		if (args.searchKey < int(rix.size())) {
		    std::cout << rix[args.searchKey] << '\n';
		}
		else {
		    throw Utils::Exception("Invalid search key");
		}
	    }
	}
	else if (args.timeIndex.size()) {
	    Utils::FilePath path(args.timeIndex);
	    path.setExtension(TimeIndex::GetIndexFileSuffix());
	    TimeIndex tix(path);
	    if (args.searchKey == -1) {
		std::cout << tix.size() << '\n';
	    }
	    else {
		std::cout << tix.findOnOrBefore(args.searchKey) << '\n';
	    }
	}
    }
    catch (Utils::Exception& ex) {
	std::cerr << ex.what() << '\n';
	return 1;
    }
    catch (std::exception& ex) {
	std::cerr << ex.what() << '\n';
	return 1;
    }
    catch (...) {
	std::cerr << "unknown exception" << '\n';
	return 1;
    }
    return 0;
}
