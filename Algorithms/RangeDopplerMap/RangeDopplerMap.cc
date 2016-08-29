#include "boost/bind.hpp"

#include <cmath>		        // for std::cos
#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Logger/Log.h"

#include "RangeDopplerMap.h"
#include "RangeDopplerMap_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
RangeDopplerMap::RangeDopplerMap(Controller& controller, Logger::Log& log)
    : Super(controller, log, kDefaultEnabled, kDefaultCpiSpan), 
      CPI_(), fft_output_(), HammingWindow_(), hamming_(), maxSpan_(1000)
{
    cpiSpan_->connectChangedSignalTo(
  	boost::bind(&RangeDopplerMap::cpiSpanChanged, this, _1));
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// RangeDopplerMap class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
RangeDopplerMap::startup()
{

    bool ok = true;
    std::stringstream converter;

    int cpiSpan = cpiSpan_->getValue();
 
    // Set the buffers to default sizes
    //
    CPI_.reset(new VsipComplexMatrix(cpiSpan, maxSpan_));
    fft_output_.reset(new VsipComplexMatrix(cpiSpan, maxSpan_));
    HammingWindow_.reset(new VsipComplexMatrix(cpiSpan, 1));

    float hamming;

    // populate the Hamming window with the correct values
    //
    for(int i = 0; i < cpiSpan; i++) {
        hamming = 0.53836 - 0.46164*cos(2.0*3.141592*(double(i)/(cpiSpan - 1)));
	(*HammingWindow_)(i , 0) = ComplexType(hamming, 0.0);
	hamming_.push_back(hamming);
    }

    return ok && Super::startup();
}

// This routine is responsible for taking a set of PRIs and performing the appropriate FFT on the set.
//
bool
RangeDopplerMap::processCPI() 
{
    static Logger::ProcLog log("processCPI", getLog());
    LOGINFO << std::endl;
    LOGDEBUG << "Process CPI with " << buffer_.size() << " msgs" << std::endl;

    int last_row = -1;
    int limit, row = 0;

    float hamming;

    size_t cpiSpan = cpiSpan_->getValue();
    if(buffer_.size() != cpiSpan) {
        LOGWARNING << "Kicking an incomplete CPI!  Expected "
		   << cpiSpan << ", but received "
		   << buffer_.size() << std::endl;
        return true;
    }

    Super::MessageQueue::iterator itr;
    size_t max_msg_size = buffer_[0]->size() / 2 ;
    for(itr = buffer_.begin(); itr != buffer_.end(); itr++) {
	if((*itr)->size() > max_msg_size) {
            max_msg_size = (*itr)->size();
	}
    }
    // Resize buffers if necessary
    //
    if(max_msg_size > maxSpan_) {
	resize(max_msg_size);
    }

    VsipComplexVector fft_buffer_(cpiSpan);

    size_t cnt;
    size_t invalid = 0;
    size_t startingSequenceNumber_ = buffer_[0]->getRIUInfo().sequenceCounter;
    // Add the PRI values composing this CPI to the appropriate buffer for performing FFTs across the PRI range
    // bins (ie, the columns of a matrix.
    //
    for(itr = buffer_.begin(), cnt = 0; 
        itr != buffer_.end() && cnt < cpiSpan; itr++, cnt++) {

	Messages::Video::Ref ref = boost::dynamic_pointer_cast<Messages::Video>(*itr);

	uint32_t seqNum = ref->getRIUInfo().sequenceCounter;

	row = seqNum - startingSequenceNumber_;
	LOGDEBUG << "Add msg: " << seqNum <<" to row: " << row << " in cpi buffer" 
                 << " starting @ msg: " << startingSequenceNumber_ 
                 << " last_row = " << last_row << std::endl;
	// pad any rows whose PRIs were dropped
	//
	for(int row_index = last_row + 1; row_index < row; row_index++) {
            LOGDEBUG << "Padding CPI(" << row_index << ") with zeroes" << std::endl;
            invalid++;
            for(size_t index = 0; index < maxSpan_; index++) {
		CPI_->row(row_index).put(index, ComplexType(0.0, 0.0)); 
            }
	}
    
        limit   = ref->size() / 2;
        hamming = hamming_[row];
      
        // populate the row with the PRI's samples
        for(int index = 0; index < limit; ++index)
            CPI_->row(row).put(index, hamming*ComplexType(ref[2*index], 
                                                          ref[2*index+1]));
        // if need be, pad the row with zeros
        for(size_t index = limit; index < maxSpan_; index++)
            CPI_->row(row).put(index, ComplexType(0.0, 0.0));
      
        last_row = row++;
    }
  
    // need to apply fft on data matrix, not on a per row basis
    ForwardFFTM fftm_(vsip::Domain<2>(cpiSpan, maxSpan_), 1.0);
    float scale = 1.0 / (cpiSpan - invalid);
    fftm_(*CPI_, *fft_output_);
    (*fft_output_) *= scale;
  
    bool rc = true;
    // For each received message of CPI, create an output message, and send the fft results for that particular
    // message. Note, we are sending magnitude, so moving from IQ data to only I data, ie, |OUT| = |MSG| / 2
    //
    size_t len = max_msg_size / 2;

    itr = buffer_.begin();

    for(itr = buffer_.begin(), cnt = 0; itr != buffer_.end() && cnt < cpiSpan;
        itr++, cnt++) {
     
        // compute appropriate index into fft output buffer
	Messages::Video::Ref ref = boost::dynamic_pointer_cast<Messages::Video>(*itr);
        row = ref->getRIUInfo().sequenceCounter - startingSequenceNumber_;

        Messages::Video::Ref out(Messages::Video::Make(getName(), ref));
        Messages::Video::Container& outputData(out->getData());

        // Convert from IQ to magnitude values
        //
	(*fft_output_).row(row) = 
            vsip::sqrt(vsip::magsq((*fft_output_).row(row)));

	// Output messages have a number of samples equal to the maximum number of samples in a PRI from this
	// CPI. Once a better CPI tracking mechanism is in place, this is irrelevant.
	//
        for(size_t i = 0; i < len; i++) {
            outputData.push_back(Messages::Video::DatumType(::rint((*fft_output_)(row, i).real())));
	}

        rc &= send(out);
    }

    return rc;
}

bool
RangeDopplerMap::resize(int max) 
{
    static Logger::ProcLog log("resize", getLog());
    LOGERROR << "Resizing from " << maxSpan_ << " to new size: " << max 
             << std::endl;
		   
    maxSpan_ = max;
    CPI_.reset(new VsipComplexMatrix(cpiSpan_->getValue(), maxSpan_));
    fft_output_.reset(new VsipComplexMatrix(cpiSpan_->getValue(), maxSpan_));

    return true;
}

bool
RangeDopplerMap::cpiSpanChanged(const Parameter::PositiveIntValue& parameter)
{
    int cpiSpan = parameter.getValue();
    HammingWindow_.reset(new VsipComplexMatrix(cpiSpan, 1));
    CPI_.reset(new VsipComplexMatrix(cpiSpan, maxSpan_));
    fft_output_.reset(new VsipComplexMatrix(cpiSpan, maxSpan_));
    // populate the Hamming window with the correct values
    float hamming;
  
    for(int i = 0; i < cpiSpan; i++) {
        hamming = 0.53836 - 0.46164*cos(2.0*3.141592*(double(i)/(cpiSpan - 1)));
	(*HammingWindow_)(i , 0) = ComplexType(hamming, 0.0);
	hamming_.push_back(hamming);
    }

    return true;
}

// Factory function for the DLL that will create a new instance of the RangeDopplerMap class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
RangeDopplerMapMake(Controller& controller, Logger::Log& log)
{
    return new RangeDopplerMap(controller, log);
}
