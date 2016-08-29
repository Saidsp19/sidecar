#ifndef SIDECAR_ALGORITHMS_MATCHEDFILTER_WORKREQUEST_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_MATCHEDFILTER_WORKREQUEST_H

#include <complex>
#include <vector>

#include "ace/Message_Block.h"
#include "ace/Message_Queue_T.h"
#include "boost/scoped_ptr.hpp"
#include <vsip/matrix.hpp>
#include <vsip/vector.hpp>

#include "Messages/Video.h"

#include "MatchedFilterTypes.h"

namespace SideCar {
namespace Algorithms {
namespace MatchedFilterUtils {

/** Collection of data values that will be used for a windowed SLC processing. Encapsulates the values that must
    not change while in use by a thread.
*/
class WorkRequest
{
public:

    /** Obtain the log device for WorkRequest objects

        \return Logger::Log reference
    */
    static Logger::Log& Log();

    /** Create a new WorkRequest object with memory provided by an ACE_Message_Block.

        \param enabledSize 

        \param corrSpan 

        \return 
    */
    static ACE_Message_Block* Make(VsipComplexVector* txPulseVec,
                                   int numFFTThreads, int fftSize);

    /** Dispose of the WorkRequest object held within an ACE_Message_Block, and release the ACE_Message_Block
	memory.

        \param data 
    */
    static void Destroy(ACE_Message_Block* data);

    /** Class method that returns a WorkRequest pointer from an ACE_Message_Block pointer.

        \param data ACE_Message_Block holding the WorkRequest data

        \return WorkRequest pointer
    */
    static WorkRequest* FromMessageBlock(ACE_Message_Block* data);

    /** Reconfigure the work request with new values. Reallocates some VSIPL objects.
     */
    void reconfigure(VsipComplexVector* txPulseVec, int numFFTThreads,
                     int fftSize);

    /** Initialize a new work request

	\param name the name of the algorithm

        \param filterStart the first sample to filter

        \param filterSpan the numbe of samples to filter

        \param main the main message to process
    */
    void beginRequest(const Messages::Video::Ref& input,
                      const Messages::Video::Ref& output,
                      size_t offset);

    /** Perform the work on the given data.
     */
    void process();

private:

    /** Consturctor. Use the Make() factory method to create new WorkRequest objects.
     */
    WorkRequest(VsipComplexVector* txPulseVec, int numFFTThreads,
                int fftSize);

    /** Destructor. Here to keep someone from manually deleting a WorkRequest object; use the Destroy() class
	method instead.
    */
    ~WorkRequest();

    VsipComplexVector* txPulseVec_;
    int fftSize_;
    Messages::Video::Ref input_;
    Messages::Video::Ref output_;
    size_t offset_;
    boost::scoped_ptr<VsipComplexVector> rxVec_;
    boost::scoped_ptr<FwdFFT> fwdFFT_;
    boost::scoped_ptr<InvFFT> invFFT_;
};

} // end namespace WindowedSLCUtils
} // end namespace Algorithms
} // end namespace SideCar

#endif
