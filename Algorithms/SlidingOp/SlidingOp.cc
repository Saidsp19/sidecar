#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "SlidingOp.h"
#include "SlidingOp_defaults.h"
#include "Utils/RunningMedian.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

static const char* kOperationNames[] = {
    "Sum",
    "Product",
    "Average",
    "Median",
    "Min",
    "Max"
};

const char* const*
SlidingOp::OperationEnumTraits::GetEnumNames()
{
    return kOperationNames;
}

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
SlidingOp::SlidingOp(Controller& controller, Logger::Log& log)
    : Super(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
      initialOffset_(Parameter::IntValue::Make("initialOffset", "Offset from message start of first window<br>"
                                               "(N < 0 will start with N emptyValue values)", kDefaultInitialOffset)),
      windowSize_(Parameter::PositiveIntValue::Make("windowSize", "Number of samples in each window",
                                                    kDefaultWindowSize)),
      emptyValue_(Parameter::IntValue::Make("emptyValue", "Value to use for non-existent samples",
                                            kDefaultEmptyValue)),
      operation_(OperationParameter::Make("operation", "Operation to perform on window samples",
                                          Operation(kDefaultOperation)))
{
    ;
}

bool
SlidingOp::startup()
{
    registerProcessor<SlidingOp,Messages::Video>(&SlidingOp::processInput);
    return registerParameter(enabled_) &&
	registerParameter(initialOffset_) &&
	registerParameter(windowSize_) &&
	registerParameter(emptyValue_) &&
	registerParameter(operation_) &&
	Super::startup();
}

namespace {

/** Simplistic wrapper around a random-access iterator from std::vector that provides for out-of-bounds indexing
    and dereferencing without errors. Returns the emptyValue value when dereferencing the iterator before or
    after valid values.
*/
class Iterator {
public:
    using const_iterator = Messages::Video::const_iterator;
    using DatumType = Messages::Video::DatumType;

    /** Constructor

        \param it std::vector iterator to use for in-bounds fetching

        \param size number of samples in the container

        \param initialOffset starting position of the iterator

        \param emptyValue value to return when out-of-bounds
    */
    Iterator(const_iterator it, ssize_t size, ssize_t initialOffset, DatumType emptyValue)
	: it_(it), index_(initialOffset), size_(size), emptyValue_(emptyValue)
	{ if (index_ > 0) it_ += index_; }

    /** Obtain the current index of the iterator.

        \return current index
    */
    ssize_t getIndex() const { return index_; }

    /** Obtain the value to use when de-referencing out-of-bound locations.

        \return empty value
    */
    DatumType getEmptyValue() const { return emptyValue_; }

    /** De-reference the iterator. If in-bounds, return a value from the underlying iterator; otherwise return
        emptyValue.

        \return iterator value
    */
    DatumType operator*() const
	{
	    if (index_ < 0 || index_ >= size_) return emptyValue_;
	    return *it_;
	}

    /** Shift the iterator by a given amount. Takes care to keep the underlying iterator within valid bounds.

        \return reference to self
    */
    Iterator& operator+=(ssize_t offset)
	{
	    ssize_t newIndex = index_ + offset;
	    if (index_ > 0) it_ -= std::min(index_, size_);
	    if (newIndex > 0) it_ += std::min(newIndex, size_);
	    index_ = newIndex;
	    return *this;
	}

    /** Increment operator. Advance the iterator by one.

        \return reference to self
    */
    Iterator& operator++()
	{
	    if (index_ >= 0 && index_ < size_) ++it_;
	    ++index_;
	    return *this;
	}

    /** Post-increment operator. Advance the iterator by one but return a copy of the iterator pointing to the
        previous location.

        \return copy of self
    */
    Iterator operator++(int)
	{
	    Iterator tmp(*this);
	    operator++();
	    return tmp;
	}

    bool operator==(const Iterator& r) const { return index_ == r.index_; }
    bool operator!=(const Iterator& r) const { return index_ != r.index_; }
    bool operator<(const Iterator& r) const { return index_ < r.index_; }
    bool operator>(const Iterator& r) const { return index_ > r.index_; }
    bool operator<=(const Iterator& r) const { return index_ <= r.index_; }
    bool operator>=(const Iterator& r) const { return index_ >= r.index_; }
    ssize_t operator-(const Iterator& r) const { return index_ - r.index_; }

private:
    const_iterator it_;
    ssize_t index_;
    const ssize_t size_;
    const DatumType emptyValue_;
};

/** Representation of a window. Maintains two iterators, one to the start of the window, and one to the end.
 */
class Window {
public:
    using const_iterator = Messages::Video::const_iterator;
    using DatumType = Messages::Video::DatumType;

    Window(const_iterator it, ssize_t size, ssize_t initialOffset, ssize_t windowSize, DatumType emptyValue)
	: begin_(it, size, initialOffset, emptyValue), end_(it, size, initialOffset + windowSize, emptyValue) {}

    Iterator& begin() { return begin_; }
    Iterator& end() { return end_; }
    DatumType getEmptyValue() const { return begin_.getEmptyValue(); }
    ssize_t getWindowSize() const { return end_ - begin_; }

private:
    Iterator begin_;
    Iterator end_;
};

struct BaseProc
{
    using DatumType = Messages::Video::DatumType;
};

/** Processor for summing values within a window.
 */
class SumProc : public BaseProc
{
public:
    SumProc(Iterator begin, Iterator end, DatumType emptyValue) : sum_(*begin++)
        { while (begin != end) sum_ += *begin++; }
    DatumType getValue() const { return sum_; }
    void advance(Iterator& begin, Iterator& end) { sum_ += (*end++ - *begin++); }
private:
    long sum_;
};

/** Processor for multiplying values within a window.
 */
class ProdProc : public BaseProc
{
public:
    ProdProc(Iterator begin, Iterator end, DatumType emptyValue) : prod_(*begin++)
	{ while (prod_ && begin != end) prod_ *= *begin++; }
    DatumType getValue() const { return prod_; }
    void advance(Iterator& begin, Iterator& end)
	{
	    if (prod_) {
		prod_ /= *begin++;
		prod_ *= *end++;
	    }
	    else {
		Iterator pos = ++begin;
		++end;
		prod_ = *pos++;
		while (prod_ && pos != end) prod_ *= *pos++;
	    }
	}
private:
    long prod_;
};

/** Template processor for tracking the min/max value within a window. The template argument should be either
    std::less_equal or std::greater_equal.
*/
template <template <typename> class T>
class TMinMaxProc : public BaseProc
{
public:

    TMinMaxProc(Iterator begin, Iterator end, DatumType emtpyValue)
	: ring_(end - begin, Pair(*begin, begin.getIndex())), bestPairIndex_(0), lastPairIndex_(0)
	{ while (begin != end) addValue(begin++); }

    /** Add a value to the ring buffer of min/max values. The algorithm below was adapted from Richard Harter's
        ascending minima algorithm C code found at http://home.tiac.net/~cri/2001/slidingmin.html. This
        templated version provides for maxima tracking as well if the temlate argument is the std::greater_equal
        functor.

        \param pos the iterator to use for the new value and index position.
    */
    void addValue(const Iterator& pos)
	{
	    DatumType value = *pos;
	    ssize_t index = pos.getIndex();
	    ssize_t death = index + ring_.size();

	    if (index == ring_[bestPairIndex_].death_) {
		++bestPairIndex_;
		if (bestPairIndex_ == ring_.size()) bestPairIndex_ = 0;
	    }

	    if (cmp_(value, ring_[bestPairIndex_].value_)) {
		ring_[bestPairIndex_].value_ = value;
		ring_[bestPairIndex_].death_ = death;
		lastPairIndex_ = bestPairIndex_;
	    }
	    else {
		while (cmp_(value, ring_[lastPairIndex_].value_)) {
		    if (lastPairIndex_ == 0) lastPairIndex_ = ring_.size();
		    --lastPairIndex_;
		}
		++lastPairIndex_;
		if (lastPairIndex_ == ring_.size()) lastPairIndex_ = 0;
		ring_[lastPairIndex_].value_ = value;
		ring_[lastPairIndex_].death_ = death;
	    }
	}

    DatumType getValue() const { return ring_[bestPairIndex_].value_; }

    void advance(Iterator& begin, Iterator& end) { ++begin; addValue(end++); }

private:

    struct Pair {
	Pair(DatumType v, ssize_t d) : value_(v), death_(d) {}
	DatumType value_;
	ssize_t death_;
    };

    std::vector<Pair> ring_;
    ssize_t bestPairIndex_;
    ssize_t lastPairIndex_;
    T<DatumType> cmp_;
};

/** Adaptation to the SumProc processor that returns average values instead of sums.
 */
class AverageProc : public SumProc
{
public:
    AverageProc(Iterator begin, Iterator end, DatumType emptyValue)
	: SumProc(begin, end, emptyValue), scale_(1.0 / (end - begin)) {}
    int getValue() const
	{ return ::round(SumProc::getValue() * scale_); }
private:
    double scale_;
};

/** Processor that tracks the median value within a window. Uses the Utils::RunningMedian class.
 */
class MedianProc : public BaseProc
{
public:
    MedianProc(Iterator begin, Iterator end, DatumType emptyValue) : runningMedian_(end - begin, emptyValue)
	{ while (begin != end) runningMedian_.addValue(*begin++); }

    int getValue() const { return runningMedian_.getMedianValue(); }
    void advance(Iterator& begin, Iterator& end) { ++begin; runningMedian_.addValue(*end++); }

private:
    Utils::RunningMedian runningMedian_;
};

/** Glue function that joins together a Window object and a data processor.
 */
template <typename T>
void
doWindows(const Messages::Video::Ref& in, Messages::Video::Ref& out, ssize_t initialOffset, ssize_t windowSize,
          typename T::DatumType emptyValue)
{
    Window window(in->begin(), in->size(), initialOffset, windowSize, emptyValue);
    T proc(window.begin(), window.end(), emptyValue);
    while (out->size() < in->size()) {
	out->push_back(proc.getValue());
	proc.advance(window.begin(), window.end());
    }
}

}

bool
SlidingOp::processInput(const Messages::Video::Ref& in)
{
    static Logger::ProcLog log("processInput", getLog());

    LOGINFO << "operation: " << operation_->getValue() << std::endl;

    if (! enabled_->getValue()) return send(in);

    Messages::Video::Ref out(Messages::Video::Make("SlidingOp", in));

    ssize_t initialOffset = initialOffset_->getValue();
    size_t windowSize = windowSize_->getValue();
    Messages::Video::DatumType emptyValue = emptyValue_->getValue();

    // Invoke the appropriate windowed processor.
    //
    switch (operation_->getValue()) {

    case kSumOp:
	doWindows<SumProc>(in, out, initialOffset, windowSize, emptyValue);
	break;

    case kProdOp:
	doWindows<ProdProc>(in, out, initialOffset, windowSize, emptyValue);
	break;

    case kMinOp:
	doWindows<TMinMaxProc<std::less_equal> >(in, out, initialOffset, windowSize, emptyValue);
	break;

    case kMaxOp:
	doWindows<TMinMaxProc<std::greater_equal> >(in, out, initialOffset, windowSize, emptyValue);
	break;

    case kAverageOp:
	doWindows<AverageProc>(in, out, initialOffset, windowSize, emptyValue);
	break;

    case kMedianOp:
	doWindows<MedianProc>(in, out, initialOffset, windowSize, emptyValue);
	break;

    default:
	LOGERROR << "unknown operation: " << operation_->getValue() << std::endl;
	break;
    }

    return send(out);
}

void
SlidingOp::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
    status.setSlot(kInitialOffset, initialOffset_->getValue());
    status.setSlot(kWindowSize, windowSize_->getValue());
    status.setSlot(kEmptyValue, emptyValue_->getValue());
    status.setSlot(kOperation, operation_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[SlidingOp::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return Algorithm::FormatInfoValue(QString("%4 Offset: %1 Window: %2 Zero: %3")
                                      .arg(int(status[SlidingOp::kInitialOffset]))
                                      .arg(int(status[SlidingOp::kWindowSize]))
                                      .arg(int(status[SlidingOp::kEmptyValue]))
                                      .arg(kOperationNames[int(status[SlidingOp::kOperation])]));
}

// Factory function for the DLL that will create a new instance of the SlidingOp class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
SlidingOpMake(Controller& controller, Logger::Log& log)
{
    return new SlidingOp(controller, log);
}
