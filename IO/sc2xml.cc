#include "boost/scoped_ptr.hpp"
#include <cmath>
#include <iostream>
#include <string>

#include "IO/FileReaderTask.h"
#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/FilePath.h"
#include "Utils/Utils.h"

using namespace SideCar;

const std::string about = "Convert SideCar data messages into XML. "
                          "Time slicing option values T0 and T1 may be specified in either relative "
                          "or absolute values (GMT). Relative values start with a '+' followed by "
                          "seconds or minutes:seconds. Absolute times start with a digit and must "
                          "be hours:minutes:seconds.";

const Utils::CmdLineArgs::OptionDef opts[] = {
    {'D', "debug", "enable root debug level", 0},
    {0, "t0", "begin processing at time T0", "T0"},
    {0, "t1", "end processing at time T1", "T1"},
    {'E', "emitted", "use message emitted timestamp instead of created", 0},
    {'c', "count", "only process COUNT records", "COUNT"},
};

const Utils::CmdLineArgs::ArgumentDef args[] = {{"IN", "path to input file"}, {0, 0}, {"OUT", "path to output file"}};

/** Simple task that places messages given to its put() method into its message queue.
 */
struct InputTask : public IO::Task {
    /** Constructor.
     */
    InputTask() : IO::Task() {}

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

struct Processor {
    Processor(int argc, char** argv);
    bool process();
    Messages::Header::Ref readNext(Time::TimeStamp& timeStamp);

    Utils::CmdLineArgs cla_;
    InputTask inputTask_;
    IO::FileReaderTask::Ref reader_;
    boost::scoped_ptr<std::ofstream> ofs_;
    std::ostream* os_;
    size_t count_;
    Time::TimeStamp t0_;
    Time::TimeStamp t1_;
    bool useEmitted_;
};

Processor::Processor(int argc, char** argv) :
    cla_(argc, argv, about, opts, sizeof(opts), args, sizeof(args)), inputTask_(), reader_(), ofs_(), os_(&std::cout),
    count_(0), t0_(Time::TimeStamp::Min()), t1_(Time::TimeStamp::Max()), useEmitted_(cla_.hasOpt("emitted"))
{
    ;
}

bool
Processor::process()
{
    // Enable verbose debugging if desired
    //
    if (cla_.hasOpt("debug")) Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    // Set the number of messages to process.
    //
    std::string value;
    if (cla_.hasOpt("count", value)) { value >> count_; }

    // Open the input file.
    //
    Utils::FilePath inputPath(cla_.arg(0));
    if (!inputPath.exists()) {
        std::cerr << "*** file '" << inputPath << "' does not exists\n";
        return false;
    }

    reader_ = IO::FileReaderTask::Make();
    reader_->next(&inputTask_);
    if (!reader_->openAndInit("", inputPath)) {
        std::cerr << "*** failed to open reader on file '" << inputPath << "'" << std::endl;
        return false;
    }

    reader_->start();

    // Create output path
    //
    std::string outputPath;
    if (cla_.hasArg(1, outputPath)) {
        ofs_.reset(new std::ofstream(outputPath.c_str()));
        os_ = ofs_.get();
    }

    // Fetch the first message so we can use its time stamp as the zero for the
    // Time::TimeStamp::ParseSpecification call.
    //
    Time::TimeStamp timeStamp;
    Messages::Header::Ref msg = readNext(timeStamp);
    if (!msg) return false;

    std::string spec;
    if (cla_.hasOpt("t0", spec)) { t0_ = Time::TimeStamp::ParseSpecification(spec, timeStamp); }

    if (cla_.hasOpt("t1", spec)) { t1_ = Time::TimeStamp::ParseSpecification(spec, t0_); }

    std::string lastProducer;
    std::string lastType;

    // Output XML file header.
    //
    *os_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         << "<!DOCTYPE file>\n"
         << "<file>\n";

    do {
        // See if we are within the time frame set by the user.
        //
        if (timeStamp < t0_)
            continue;
        else if (timeStamp > t1_)
            break;

        // See if this message has a different producer name or message type.
        //
        std::string producer(msg->getGloballyUniqueID().getProducerName());
        std::string type(msg->getMetaTypeInfo().getName());
        bool split = false;

        if (producer != lastProducer) {
            if (lastProducer.size())
                std::clog << "*** warning: diffent producers found in file: " << producer << " and " << lastProducer
                          << "\n";
            split = true;
        } else if (lastType != type) {
            if (lastType.size())
                std::clog << "*** warning: diffent message types found in "
                             "file: "
                          << type << " and " << lastType << "\n";
            split = true;
        }

        if (split) {
            // Close the last message block.
            //
            if (lastProducer.size()) *os_ << "</group>\n";

            lastProducer = producer;
            lastType = type;

            // Open a new message block with the new information.
            //
            *os_ << "<group producer=\"" << producer << "\" type=\"" << type << "\">\n";
        }

        // Write out the XML data and stop if we've written enough.
        //
        *os_ << msg->xmlPrinter() << '\n';

        // See if we have a counter to watch.
        //
        if (count_) {
            if (--count_ == 0) { break; }
        }

    } while ((msg = readNext(timeStamp)));

    // NOTE: order is important here. We may exit above while the FileReaderTask is still running, perhaps blocked
    // trying to add another message to the InputTask's message queue. We deactivate that queue to wake up any blocked
    // thread, and then properly shutdown the FileReaderTask and its thread.
    //
    inputTask_.msg_queue()->deactivate();
    reader_->close(1);

    // Close the last message block and if writing to a file, close it too.
    //
    *os_ << "</group>\n</file>\n";
    if (ofs_) ofs_->close();

    return true;
}

Messages::Header::Ref
Processor::readNext(Time::TimeStamp& timeStamp)
{
    // Read the next message from the input queue.
    //
    Messages::Header::Ref msg;
    ACE_Message_Block* data;
    if (inputTask_.getq(data, 0) == -1) return msg;

    // If this is an EOF signal from the FileReaderTask then signal that we are done.
    //
    IO::MessageManager mgr(data);
    if (mgr.isShutdownRequest()) return msg;

    // Get the native message and fetch the appropriate time stamp.
    //
    msg = mgr.getNative();
    timeStamp = useEmitted_ ? msg->getEmittedTimeStamp() : msg->getCreatedTimeStamp();

    return msg;
}

int
main(int argc, char** argv)
{
    Processor processor(argc, argv);
    return processor.process() ? 0 : 1;
}
