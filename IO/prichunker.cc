#include <cmath>
#include <iostream>
#include <string>

#include "ace/FILE_Connector.h"
#include "boost/algorithm/minmax.hpp"
#include "boost/algorithm/minmax_element.hpp"
#include "boost/lexical_cast.hpp"

#include "IO/FileReaderTask.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Messages/RadarConfig.h"
#include "Messages/Video.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/FilePath.h"
#include "Utils/Utils.h"

using namespace SideCar;

const std::string about = "Load and print information about a PRI file.";

const Utils::CmdLineArgs::OptionDef opts[] = {
    { 'D', "debug", "enable root debug level", 0 },
};

const Utils::CmdLineArgs::ArgumentDef args[] = {
    { "FILE", "path to input file" },
    { "SIZE", "size of output files in MBytes" },
};

/** Simple task that places messages given to its put() method into its message queue.
 */
struct PRIChunkerInputTask : public IO::Task
{
    /** Constructor.
     */
    PRIChunkerInputTask() : IO::Task() {}

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

struct PRIChunkerOutputTask : public IO::Task
{
    /** Constructor.
     */
    PRIChunkerOutputTask() : IO::Task() {}

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

    size_t fileSize = boost::lexical_cast<int>(cla.arg(1)) * 1048576;

    // Locate a radar configuration file for the data file.
    //
    Utils::FilePath configPath(filePath);
    configPath.setExtension("xml");
    if (configPath.exists()) {
	std::clog << cla.progName() << ": using '" << configPath
		  << "' for radar config\n";
	Messages::RadarConfig::SetConfigurationFilePath(configPath);
    }

    // Locate last '.' so we can insert file index
    //
    std::string fileString = cla.arg(0);
    std::string::size_type lastDotLoc = fileString.rfind(".");
    
    // Create a new FileReaderTask object.
    //
    IO::FileReaderTask::Ref reader(IO::FileReaderTask::Make());

    // Create a new FileWriterTask object.
    //
    IO::FileWriterTask::Ref writer(IO::FileWriterTask::Make());

    // Create a new input task to collect the messages read in from the reader task. The reader task will invoke
    // InputTask::put() for each message it reads in.
    //
    PRIChunkerInputTask input;
    reader->next(&input);

    if (! reader->openAndInit("Video", filePath.c_str())) {
	std::cerr << "*** failed to open/start reader on file '"
		  << cla.arg(0) << "'" << std::endl;
	return 1;
    }

    reader->start();

    size_t outputFileSize = 0;
    int    outputFileIndex = 1;

    // Read from the InputTask message queue until an error, or we receive a stop signal from the reader task
    // that there is no more data to processs.
    //
    ACE_Message_Block* data;
    while (input.getq(data, 0) != -1) {
	IO::MessageManager mgr(data);
	if (mgr.isShutdownRequest()) break;

	if (! outputFileSize) {

	    std::string outfileString = cla.arg(0);

	    if (lastDotLoc != std::string::npos) {
		outfileString.insert(lastDotLoc, "_");
		outfileString.insert(lastDotLoc+1, boost::lexical_cast<std::string>(outputFileIndex++));
	    }
	    else	
		outfileString +=  "_" + boost::lexical_cast<std::string>(outputFileIndex++);

	    Utils::FilePath outputFilePath(outfileString);
	    
	    std::cout << "Writing " << outputFilePath << " : ";
	    
	    if (! writer->openAndInit("Video", outputFilePath.c_str())) {
		std::cerr << "*** failed to open/start writer on file '"
			  << outputFilePath << "'" << std::endl;
		return 1;
	    }
	    
	    writer->put(data->duplicate());
	    outputFileSize += mgr.getMessageSize();
	}
	else {
	    writer->put(data->duplicate());
	    outputFileSize += mgr.getMessageSize();
	    if (outputFileSize > fileSize) {
		
		std::cout << outputFileSize << std::endl;
		
		outputFileSize = 0;
	    }
	}
    }

    writer->close(1);
 
    std::cout << outputFileSize << std::endl;
    std::cout << "Finished" << std::endl;
    return 0;
}
