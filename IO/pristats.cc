// This program processes select messages and samples from a PRI and computes the average magnitude squared, the
// average power, and median power for the given sample range in each message of the given message range.
//

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
#include "Messages/Video.h"
#include "Utils/CmdLineArgs.h"
#include "Utils/FilePath.h"
#include "Utils/Utils.h"

using namespace SideCar;

const std::string about = "Print the average magnitude squared, power, and median power for certain samples and messages of a PRI file.";

const Utils::CmdLineArgs::OptionDef opts[] = {
    { 'D', "debug", "enable root debug level", 0 },
    { 's', "start_sample", "start index into a PRI for slicing out samples (defaults to 0)", "START_SAMPLE" },
    { 'e', "end_sample", "end index into a PRI for slicing out samples (defaults to -1)", "END_SAMPLE" },
    { 'S', "start_message", "starting PRI message number for analysis (defaults to 0)", "MSG_START" },
    { 'E', "end_message", "ending PRI message number for analysis (defaults to 0)", "MSG_END" }
};

const Utils::CmdLineArgs::ArgumentDef args[] = {
    { "INFILE", "path to input file" }
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

double
az(uint32_t shaftEncoding)
{
    return shaftEncoding * 360.0 / 65536.0;
}

int
main(int argc, char** argv)
{
    Utils::CmdLineArgs cla(argc, argv, about, opts, sizeof(opts), args,
                           sizeof(args));
    if (cla.hasOpt("debug"))
	Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);

    Utils::FilePath infilePath(cla.arg(0));

    if (! infilePath.exists()) {
	std::cerr << "*** file '" << infilePath << "' does not exists\n";
	return 1;
    }

    std::string start_message_str(""), end_message_str(""),
        start_sample_str(""), end_sample_str("");

    // Create a new FileReaderTask object.
    //
    IO::FileReaderTask::Ref counter(IO::FileReaderTask::Make());

    // Create a new input task to collect the messages read in from the reader task. The reader task will invoke
    // InputTask::put() for each message it reads in.
    //
    PRIInfoInputTask input;
    counter->next(&input);

    if (! counter->openAndInit("Video", infilePath.c_str())) {
	std::cerr << "*** failed to open/start counter on file '"
		  << cla.arg(0) << "'" << std::endl;
	return 1;
    }

    counter->start();

    size_t messageCounter_ = 0;

    // Read from the InputTask message queue until an error, or we receive a stop signal from the reader task
    // that there is no more data to processs.
    //
    ACE_Message_Block* data;
    while (input.getq(data, 0) != -1) {
	IO::MessageManager mgr(data);
	if (mgr.isShutdownRequest()) break;
	messageCounter_++;
    }

    if (! messageCounter_) {
	std::cerr << "*** no messages found ***\n";
	return 1;
    }

    // Store the number of messages in the PRI file and reset the counter
    //
    size_t numMessages = messageCounter_;
    messageCounter_ = 0;

    // Close the InputTask responsible for counting PRIs
    //
    counter->close();

    // Calculate any user-defined ranges for processing
    //
    int start_msg_ = 0;
    int end_msg_   = numMessages - 1;

    if(cla.hasOpt("start_message", start_message_str)) {
        start_msg_ = atoi(start_message_str.c_str());
        // Handle negative indices
        //
        if(start_msg_ < 0) { 
            start_msg_ = numMessages + start_msg_;
        }
    }
    if(cla.hasOpt("end_message", end_message_str)) {
        end_msg_ = atoi(end_message_str.c_str());
        // Handle negative indices
        //
        if(end_msg_ < 0) { 
            end_msg_ = numMessages + end_msg_;
        }
    }


    int start_sample_ = 0;
    int end_sample_ = -1;

    // Negative indices for samples must be handled once the mesage arrives and is processed, so for now record
    // only the user-defined values.
    //
    if(cla.hasOpt("start_sample", start_sample_str)) {
        start_sample_ = atoi(start_sample_str.c_str());
    }

    if(cla.hasOpt("end_sample", end_sample_str)) {
        end_sample_ = atoi(end_sample_str.c_str());
    }

    // Create new Task for reading the input again
    //
    
    IO::FileReaderTask::Ref reader(IO::FileReaderTask::Make());
    
    reader->next(&input);

    if(! reader->openAndInit("Video", infilePath.c_str())) {
	std::cerr << "*** failed to open/start counter on file '"
		  << cla.arg(0) << "'" << std::endl;
	return 1;
    }

    reader->start();

    int start_index_ = start_sample_;
    int end_index_   = end_sample_;

    std::cerr << "Grabbing messages " << start_msg_ << " through "
	      << end_msg_ << " and samples " << start_sample_ << " through "
	      << end_sample_ << "\n";

    // Read in the contents of the PRI file, keeping the user-defined ranges of messages (for the file) and
    // samples (for each PRI)
    //

    while(input.getq(data, 0) != -1) {
        IO::MessageManager mgr(data);

        if(mgr.isShutdownRequest()) break;

        Messages::Video::Ref video = mgr.getNative<Messages::Video>();

        if(int(messageCounter_) >= start_msg_ && 
           int(messageCounter_) <= end_msg_) {
            // Compute the appropriate sample indices into this PRI
            //
            if(start_sample_ < 0) {
                start_index_ = video->size() + 2*start_sample_;
            } else {
                start_index_ = 2*start_sample_;
            }
	
            if(end_sample_ < 0) {
                end_index_ = video->size() + 2*end_sample_;
            } else {
                end_index_ = 2*end_sample_;
            }

            // Validate indices
            //

            if(start_index_ < 0 || start_index_ >= int(video->size()) ||
               end_index_ < 0 || end_index_ >= int(video->size()) ||
               end_index_ < start_index_) {
                std::cout << "Error: Illegal sample indices: ["
                          << start_index_ << ", " << end_index_
                          << "] with message size = " << video->size()
                          << "\n";
	  
                return 1;
            }

            size_t length_ = end_index_ - start_index_ + 2;
	
            int sample_cnt_ = length_ / 2;

            std::vector<float> vec_;
            Messages::Video::Container& out(video->getData());
            for(size_t index_ = 0; index_ < length_; index_++) {
                out.push_back(video[start_index_ + index_]);
            }
            // Record metadata about this PRI slice
            //
            float mag_sqrd_sum_ = 0.0;
            for(int index_  = 0; index_ < sample_cnt_; index_++) {
                // Convert 16-bit int to float value
                //
                float   r = video[start_index_ + 2*index_ ] / 2.828427125;
                float   c = video[start_index_ + 2*index_ + 1] / 2.828427125;
                float mag_sqrd =  r*r + c*c;
                mag_sqrd_sum_ += mag_sqrd;

                vec_.push_back(mag_sqrd);
            }
            // Sort the vector of squared mags. Uses simple selection sort because we are sorting a relatively small
            // number of samples.
            //
            for(size_t i = 0; i < vec_.size() - 2; i++) {
                size_t min_ = i;
                for(size_t j = i + 1; j < vec_.size() - 1; j++) {
                    if(vec_[j] < vec_[min_]) {
                        min_ = j;
                    }
                }
                float tmp_   = vec_[i];
                vec_[i]    = vec_[min_];
                vec_[min_] = tmp_;
            }

            for(size_t i = 0; i < vec_.size(); i++) {
                std::cout << vec_[i] << "\n";
            }

            float median = vec_[int(vec_.size() / 2)];
            std::cout << mag_sqrd_sum_ / sample_cnt_ << ", "
                      << 10.0*::log10(20.0*mag_sqrd_sum_ / sample_cnt_) << ", "
                      << 10.0*::log10(20.0*median) << "\n";
        } 
        messageCounter_++;
    }

    reader->close();

    return 0;
}
