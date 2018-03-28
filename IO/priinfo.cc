#include <cmath>
#include <iostream>
#include <string>

#include "ace/FILE_Connector.h"
#include "boost/algorithm/minmax.hpp"
#include "boost/algorithm/minmax_element.hpp"

#include "Configuration/Loader.h"
#include "IO/FileReaderTask.h"
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
    {'C', "noconfig", "do not attempt to load a RadarConfig file", 0},
    {'D', "debug", "enable root debug level", 0},
    {'d', "data", "print PRI data for each record", 0},
    {'g', "gatedelta", "min change in message size to report", "GATES"},
    {'X', "xml", "write messages as XML records", 0},
    {'h', "header", "print PRI header for each record", 0},
};

const Utils::CmdLineArgs::ArgumentDef args[] = {{"FILE", "path to input file"}};

/** Simple task that places messages given to its put() method into its message queue.
 */
struct PRIInfoInputTask : public IO::Task {
    /** Constructor.
     */
    PRIInfoInputTask() : IO::Task() {}

    /** Override of IO::Task method. Puts message into internal message queue.

        \param block message data

        \param timeout amount of time to spend trying to insert

        \return true if successful
    */
    bool deliverControlMessage(ACE_Message_Block* block, ACE_Time_Value* timeout) { return putq(block, timeout) != -1; }

    /** Override of IO::Task method. Puts message into internal message queue.

        \param block message data

        \param timeout amount of time to spend trying to insert

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* block, ACE_Time_Value* timeout) { return putq(block, timeout) != -1; }
};

double
az(uint32_t shaftEncoding)
{
    return shaftEncoding * 360.0 / 65536.0;
}

int
main(int argc, char** argv)
{
    Utils::CmdLineArgs cla(argc, argv, about, opts, sizeof(opts), args, sizeof(args));
    if (cla.hasOpt("debug")) Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    Utils::FilePath filePath(cla.arg(0));
    if (!filePath.exists()) {
        std::cerr << "*** file '" << filePath << "' does not exists\n";
        return 1;
    }

    // Locate a radar configuration file for the data file.
    //
    if (!cla.hasOpt("noconfig")) {
        Utils::FilePath configPath(filePath);
        configPath.setExtension("xml");
        if (configPath.exists()) {
            Configuration::Loader loader;
            std::clog << cla.progName() << ": using '" << configPath << "' for radar config\n";
            loader.loadRadarConfig(configPath);
        }
    }

    bool xml = cla.hasOpt("xml");
    bool printData = cla.hasOpt("data");
    bool printHeader = cla.hasOpt("header");
    int gateDelta = 2;
    if (cla.hasOpt("gatedelta")) cla.opt("gatedelta")[0] >> gateDelta;

    // Create a new FileReaderTask object.
    //
    IO::FileReaderTask::Ref reader(IO::FileReaderTask::Make());

    // Create a new input task to collect the messages read in from the reader task. The reader task will invoke
    // InputTask::put() for each message it reads in.
    //
    PRIInfoInputTask input;
    reader->next(&input);

    if (!reader->openAndInit("", filePath.c_str())) {
        std::cerr << "*** failed to open/start reader on file '" << cla.arg(0) << "'" << std::endl;
        return 1;
    }

    reader->start();

    Time::TimeStamp deltaSum;
    Time::TimeStamp lastTime;

    size_t messageCounter = 0;
    size_t gateCountMin = 0;
    size_t gateCountMax = 0;

    Time::TimeStamp startTime = Time::TimeStamp::Max();
    Time::TimeStamp endTime = Time::TimeStamp::Min();
    Time::TimeStamp totalTime = Time::TimeStamp::Min();

    short powerMin = 0;
    short powerMax = 0;

    uint32_t lastSequenceNumber = 0;
    uint32_t lastGateCount = 0;
    uint32_t duplicateCounter = 0;
    size_t duplicateMessages = 0;
    size_t droppedMessages = 0;

    // Read from the InputTask message queue until an error, or we receive a stop signal from the reader task
    // that there is no more data to processs.
    //
    ACE_Message_Block* data;
    while (input.getq(data, 0) != -1) {
        IO::MessageManager mgr(data);
        if (mgr.isShutdownRequest()) break;

        using iterator = Messages::Video::const_iterator;
        Messages::Header::Ref msg = mgr.getNative();
        if (!msg) break;

        if (xml) {
            if (!messageCounter) {
                std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          << "<!DOCTYPE msgs>\n";

                std::cout << "<msgs producer=\"" << msg->getGloballyUniqueID().getProducerName() << "\" type=\""
                          << msg->getMetaTypeInfo().getName() << "\">\n";
            }
            std::cout << msg->xmlPrinter() << '\n';
        } else if (printData) {
            std::cout << msg->dataPrinter() << '\n';
        } else if (printHeader) {
            std::cout << msg->headerPrinter() << '\n';
        }

        if (!messageCounter++) {
            lastTime = msg->getCreatedTimeStamp();
            lastSequenceNumber = msg->getMessageSequenceNumber();
            if (msg->getMetaTypeInfo().getKey() == Messages::MetaTypeInfo::Value::kVideo) {
                Messages::Video::Ref video = mgr.getNative<Messages::Video>();
                std::pair<iterator, iterator> mm(boost::minmax_element(video->begin(), video->end()));
                powerMin = *mm.first;
                powerMax = *mm.second;
                gateCountMin = video->size();
                gateCountMax = video->size();
                lastGateCount = video->size();
            }
        } else {
            Time::TimeStamp delta(msg->getCreatedTimeStamp());
            delta -= lastTime;
            deltaSum += delta;
            lastTime = msg->getCreatedTimeStamp();
            if (lastTime < startTime) startTime = lastTime;
            if (lastTime > endTime) endTime = lastTime;
            uint32_t sequenceNumber = msg->getMessageSequenceNumber();

            if (msg->getMetaTypeInfo().getKey() == Messages::MetaTypeInfo::Value::kVideo) {
                Messages::Video::Ref video = mgr.getNative<Messages::Video>();
                std::pair<iterator, iterator> mm(boost::minmax_element(video->begin(), video->end()));
                if (*mm.first < powerMin) powerMin = *mm.first;
                if (*mm.second > powerMax) powerMax = *mm.second;

                // sequenceNumber = video->getRIUInfo().sequenceCounter;
                uint32_t gateCount = video->size();
                if (gateCount < gateCountMin) {
                    std::clog << "** MIN gate count changed - Msg: " << messageCounter
                              << " Az: " << az(video->getRIUInfo().shaftEncoding) << " old: " << gateCountMin
                              << " new: " << video->size() << std::endl;
                    gateCountMin = video->size();
                } else if (gateCount > gateCountMax) {
                    std::clog << "** MAX gate count changed - Msg " << messageCounter
                              << " Az: " << az(video->getRIUInfo().shaftEncoding) << " old: " << gateCountMax
                              << " new: " << video->size() << std::endl;
                    gateCountMax = video->size();
                }

                if (gateCount > lastGateCount + gateDelta) {
                    std::clog << "** Gate count changed - Msg " << messageCounter
                              << " Az: " << az(video->getRIUInfo().shaftEncoding) << " old: " << lastGateCount
                              << " new: " << gateCount << std::endl;
                } else if (gateCount < lastGateCount - gateDelta) {
                    std::clog << "** Gate count changed - Msg " << messageCounter
                              << " Az: " << az(video->getRIUInfo().shaftEncoding) << " old: " << lastGateCount
                              << " new: " << gateCount << std::endl;
                }

                lastGateCount = gateCount;
            }

            if (sequenceNumber != lastSequenceNumber + 1) {
                if (sequenceNumber == lastSequenceNumber) {
                    ++duplicateMessages;
                    ++duplicateCounter;
                } else {
                    if (duplicateCounter > 0) {
                        std::clog << "** Dup msg - Msg: " << (messageCounter - duplicateCounter - 1)
                                  << " sequenceNumber: " << lastSequenceNumber << " occurances: " << duplicateCounter
                                  << std::endl;
                        duplicateCounter = 0;
                    }

                    uint32_t delta = sequenceNumber - lastSequenceNumber - 1;
                    droppedMessages += delta;
                    std::clog << "** Gap of " << delta << " - Msg: " << messageCounter << " current: " << sequenceNumber
                              << " last: " << lastSequenceNumber << std::endl;
                }
            }

            lastSequenceNumber = sequenceNumber;
        }
    }

    if (!messageCounter) {
        std::cerr << "*** no messages found ***\n";
        return 1;
    }

    if (xml) std::cout << "</msgs>\n";

    double frequency = 0.0;
    if (messageCounter > 1) {
        double deltaAverage = deltaSum.asDouble() / (messageCounter - 1);
        if (deltaAverage) frequency = ::rint(1.0 / deltaAverage);
    }

    double radialsPerScan = ::rint((M_PI * 2.0) / Messages::RadarConfig::GetBeamWidth());

    totalTime = endTime - startTime;

    std::cout << "    # Messages: " << messageCounter << "\n   Drop Seq#: " << droppedMessages << " ("
              << (droppedMessages * 100.0 / messageCounter) << "%)"
              << "\n   Dupe Seq#: " << duplicateMessages << " (" << (duplicateMessages * 100.0 / messageCounter) << "%)"
              << "\n     # Scans: " << (messageCounter / radialsPerScan) << "\n   Frequency: " << frequency << " Hz"
              << "\n  Beam Width: " << Utils::radiansToDegrees(Messages::RadarConfig::GetBeamWidth()) << " degrees ("
              << Messages::RadarConfig::GetBeamWidth() << " radians)"
              << "\n     Radials: " << radialsPerScan << "\n   Min Power: " << powerMin
              << "\n   Max Power: " << powerMax << "\n   Min Gates: " << gateCountMin
              << "\n   Max Gates: " << gateCountMax << "\n  Start Time: " << startTime << "\n    End Time: " << endTime
              << "\n    Duration: " << totalTime.hhmmss(false) << "\n";
    return 0;
}
