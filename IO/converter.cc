#include <string>

#include "ace/FILE_Connector.h"

#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"
#include "Messages/Video.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/Utils.h"

#include "FileReaderTask.h"
#include "FileWriterTask.h"
#include "MessageManager.h"

using namespace SideCar;
using namespace SideCar::IO;
using namespace SideCar::Messages;

const std::string about = "PRI format converter";

const Utils::CmdLineArgs::OptionDef options[] = {{'d', "debug", "turn on root-level debug", 0},
                                                 {'f', "frequency", "rate at which the timmestamps change ", "FREQ"}};

const Utils::CmdLineArgs::ArgumentDef args[] = {
    {"INPUT", "path to input file"}, {"OUTPUT", "path to output file"}, {0, 0}};

/** Simple task that places messages given to its put() method into its message queue.
 */
struct ConverterInputTask : public IO::Task {
    /** Constructor
     */
    ConverterInputTask() : IO::Task() {}

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

int
main(int argc, char** argv)
{
    float frequency = 180.0;
    std::string value("");

    Utils::CmdLineArgs cla(argc, argv, about, options, sizeof(options), args, sizeof(args));

    if (cla.hasOpt("frequency", value))
        if (!(value >> frequency) || frequency <= 0.0) cla.usage("invalid 'frequency' value");

    if (cla.hasOpt("debug")) Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    float timeRate = 1.0 / frequency;
    Time::TimeStamp clock(0.0);

    // Create a new FileReaderTask object.
    //
    IO::FileReaderTask::Ref reader(IO::FileReaderTask::Make());

    // Create a new input task to collect the messages read in from the reader task. The reader task will invoke
    // InputTask::put() for each message it reads in.
    //
    ConverterInputTask input;
    reader->next(&input);
    if (!reader->openAndInit("Video", cla.arg(0))) {
        std::cerr << "*** failed to open/start reader on file '" << cla.arg(1) << "'" << std::endl;
        return 1;
    }

    // Create a new FileWriter object
    //
    IO::FileWriterTask::Ref writer(IO::FileWriterTask::Make());
    if (!writer->openAndInit("Video", cla.arg(1))) {
        std::cerr << "*** failed to open/start writer on file '" << cla.arg(2) << "'" << std::endl;
        return 1;
    }

    reader->start();
    ACE_Message_Block* data = 0;
    while (input.getq(data, 0) != -1) {
        IO::MessageManager mgr(data);
        if (mgr.isShutdownRequest()) {
            writer->put(data->duplicate());
            writer->close(1);
            break;
        }

        Video::Ref in(mgr.getNative<Video>());
        Video::Ref out(Video::Make("converter", in));
        out->getData() = in->getData();
        out->getRIUInfo().shaftEncoding = in->getRIUInfo().sequenceCounter - 1;

        out->setCreatedTimeStamp(clock);
        clock += timeRate;

        writer->put(MessageManager(out).getMessage(), 0);
    }

    return 0;
}
