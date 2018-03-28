#include <cmath>
#include <iostream>
#include <string>

#include "ace/FILE_Connector.h"

#include "IO/MessageManager.h"
#include "IO/Writers.h"
#include "Logger/Log.h"
#include "Messages/RadarConfig.h"
#include "Messages/TSPI.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/FilePath.h"
#include "Utils/Utils.h"

using namespace SideCar;

const std::string about = "Convert CSV truth values into TSPI records for "
                          "playback.";

const Utils::CmdLineArgs::OptionDef opts[] = {{'c', "config", "use FILE for radar configuration", "FILE"},
                                              {'d', "debug", "enable debug logging", 0},
                                              {'t', "timeoffset", "time offset to use for timestamps", "SECS"}};

const Utils::CmdLineArgs::ArgumentDef args[] = {{"IN", "path to input file"}, {"OUT", "path to output file"}};

int
main(int argc, char** argv)
{
    Logger::Log& log = Logger::Log::Find("truthgen");
    Utils::CmdLineArgs cla(argc, argv, about, opts, sizeof(opts), args, sizeof(args));

    if (cla.hasOpt("debug"))
        log.setPriorityLimit(Logger::Priority::kDebug);
    else
        log.setPriorityLimit(Logger::Priority::kInfo);

    Utils::FilePath filePath(cla.arg(0));
    if (!filePath.exists()) {
        LOGERROR << "input file '" << filePath << "' does not exists" << std::endl;
        return 1;
    }

    // Locate a radar configuration file for the data file.
    //
    std::string value;
    if (cla.hasOpt("config", value)) {
        LOGINFO << "using '" << value << "' for radar configuration" << std::endl;
        Utils::FilePath configPath(value);
        Messages::RadarConfig::SetConfigurationFilePath(configPath);
    }

    // Create a new FileWriter object and then 'connect' it to the file path given on the command line.
    //
    IO::FileWriter::Ref writer(IO::FileWriter::Make());
    ACE_FILE_Addr address(cla.arg(1).c_str());
    ACE_FILE_Connector fd(writer->getDevice(), address);
    writer->getDevice().truncate(0);

    double timeOffset = 0.0;
    if (cla.hasOpt("timeoffset", value)) {
        value >> timeOffset;
        LOGINFO << "applying " << timeOffset << " seconds to all timestamps" << std::endl;
    }

    std::ifstream is(cla.arg(0).c_str());
    double latitude, longitude, timeStamp;
    int height, id;
    char c1, c2, c3, c4;
    int counter = 0;
    while (is >> longitude >> c1 >> latitude >> c2 >> height >> c3 >> id >> c4 >> timeStamp) {
        LOGDEBUG << "lat: " << latitude << " lo: " << longitude << " hgt: " << height << " id: " << id
                 << " time: " << timeStamp << std::endl;

        std::ostringstream os;
        os << id;
        Time::TimeStamp time(timeStamp + timeOffset);
        Messages::TSPI::Ref msg = Messages::TSPI::MakeLLH("TruthGen", os.str(), time.asDouble(), latitude, longitude,
                                                          Utils::feetToMeters(height));

        LOGDEBUG << "new time stamp: " << time << std::endl;
        msg->setCreatedTimeStamp(time);

        IO::MessageManager mgr(msg);
        if (!writer->write(mgr)) {
            LOGERROR << "failed to write TSPI message - " << writer->getLastError() << std::endl;
            break;
        }
    }

    LOGINFO << "wrote " << counter << " TSPI records" << std::endl;

    return 0;
}
