#include <cmath>		// for ::sqrt()

#include "boost/bind.hpp"

#include "Logger/Log.h"

#include "Exception.h"
#include "RunningMedian.h"

using namespace Utils;

Logger::Log&
RunningMedian::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("Utils.RunningMedian");
    return log_;
}

RunningMedian::RunningMedian(size_t windowSize, double initialValue)
    : oldest_(0), youngest_(0), fingers_(), windowSize_(0)
{
    setWindowSize(windowSize, initialValue);
}

RunningMedian::~RunningMedian()
{
    releaseNodes();
}

void
RunningMedian::setWindowSize(size_t windowSize, double initialValue)
{
    Logger::ProcLog log("setWindowSize", Log());
    LOGINFO << "windowSize: " << windowSize << " initialValue: " << initialValue << std::endl;

    if (windowSize == 0) {
	throw Utils::Exception("windowSize must be non-zero");
    }

    // If no change in size, just reset to the initial value.
    //
    if (windowSize_ == windowSize) {
	clear(initialValue);
	return;
    }

    // Release any existing Node objects and prepare to create new ones for the new window size.
    //
    if (oldest_) {
	releaseNodes();
    }

    windowSize_ = windowSize;
    median_ = initialValue;
    sum_ = initialValue * windowSize;
    minValue_ = maxValue_ = initialValue;
    isOdd_ = windowSize & 1;

    // Calculate how many Node objects there are between finger Nodes.
    //
    fingerSpan_ = size_t(::sqrt(windowSize));
    size_t fingerIndex = 0;

    LOGDEBUG << "fingerSpan: " << fingerSpan_ << std::endl;

    // Calculate the finger index where the median value would be found. TODO: change to locate the closest
    // finger in distance.
    //
    size_t midPoint;
    midPoint = (windowSize - 1) / 2;

    // Calculate which finger Nodes to use for accessing the median entry, and how many Nodes we must traverse
    // before we reach it.
    //
    medianFingerIndex_ = size_t(midPoint / fingerSpan_);
    medianFingerOffset_ = midPoint - medianFingerIndex_ * fingerSpan_;
    LOGDEBUG << "midPoint: " << midPoint << " medianFingerIndex: " << medianFingerIndex_
             << " medianFingerOffset: " << medianFingerOffset_ << std::endl;

    // Create our value Nodes, initialized to the given value and properly linked up.
    //
    for (size_t index = 0; index < windowSize; ++index) {

	Node* node = new Node;
	node->value_ = initialValue;
	node->younger_ = 0;
	node->bigger_ = 0;

	if (! oldest_) {
	    oldest_ = node;
	}
	else {
	    youngest_->bigger_ = node;
	    youngest_->younger_ = node;
	}

	node->smaller_ = youngest_;
	youngest_ = node;

	// Periodically create a finger entry. NOTE: since ::sqrt(1) is 1, we will always have at least one
	// finger Node object.
	//
	if (index == fingerIndex) {
	    fingers_.push_back(node);
	    fingerIndex += fingerSpan_;
	}
    }
}

void
RunningMedian::releaseNodes()
{
    while (oldest_) {
	Node* tmp = oldest_;
	oldest_ = tmp->younger_;
	delete tmp;
    }

    oldest_ = 0;
    youngest_ = 0;
    fingers_.clear();
}

RunningMedian::Node*
RunningMedian::storeValue(double value)
{
    // Unlink the oldest Node, and relink as the youngest.
    //
    Node* tmp = oldest_;

    if (value != tmp->value_) {
	sum_ -= tmp->value_;
	sum_ += value;

	if (tmp->value_ == maxValue_ && ! tmp->bigger_ && tmp->smaller_)
	    maxValue_ = tmp->smaller_->value_;

	if (tmp->value_ == minValue_ && ! tmp->smaller_ && tmp->bigger_)
	    minValue_ = tmp->bigger_->value_;

	tmp->value_ = value;
    }

    if (tmp->younger_) {
	oldest_ = tmp->younger_;
	tmp->younger_ = 0;
	youngest_->younger_ = tmp;
	youngest_ = tmp;
    }

    return tmp;
}

double
RunningMedian::addValue(double newValue)
{
    static Logger::ProcLog log("addValue", Log());
    LOGINFO << "newValue: " << newValue << std::endl;

    Node* left = 0;
    Node* right = 0;

    // Check if we can insert before the first Node.
    //
    if (newValue <= fingers_.front()->value_) {
	right = fingers_.front();
    }
    else {

	// Going backwards, locate the first finger Node that has a value less than the new one.
	//
	size_t fingerIndex = findLargestFingerSmallerThan(newValue);

	// Now walk forwards from the found finger Node
	//
	left = fingers_[fingerIndex];
	right = left->bigger_;
	while (right && newValue > right->value_) {
	    left = right;
	    right = right->bigger_;
	}
    }

    // At this point, left or right may be NULL, but never both at the same time.
    //
    double oldestValue = oldest_->value_;
    LOGDEBUG << "left: " << left << " right: " << right << " oldest: " << oldest_ << " oldestValue: "
             << oldestValue << std::endl;

    LOGDEBUG3 << "BEFORE" << std::endl;
    if (log.showsDebug3()) dump();

    if (left == oldest_ || right == oldest_ || oldestValue == newValue) {

	// We want to insert at a point where we would reuse the oldest Node. Since it is already in the proper
	// ordered linked-list position, just make it the youngest node and update its value and we're done.
	//
	LOGDEBUG << "reusing oldest entry" << std::endl;
	storeValue(newValue);

	// Update min/max values, taking care of corner cases.
	//
	if (left == oldest_) {
	    maxValue_ = newValue;
	    if (windowSize_ == 1)
		minValue_ = newValue;
	}

	if (right == oldest_) {
	    minValue_ = newValue;
	    if (windowSize_ == 1)
		maxValue_ = newValue;
	}
    }
    else {

	bool shiftBigger = false;
	std::vector<size_t> shifts;
	if (oldestValue < newValue) {
	    shiftBigger = true;
	    size_t oldFingerIndex = findFingerIndexAtOrBeforeOldest();
	    LOGDEBUG << "oldestValue < newValue - oldFingerIndex: " << oldFingerIndex << std::endl;

	    for (; oldFingerIndex < fingers_.size() && fingers_[oldFingerIndex]->value_ < newValue;
                 ++oldFingerIndex) {
		Node* tmp = fingers_[oldFingerIndex];
		shifts.push_back(oldFingerIndex);
		LOGDEBUG << "will shift " << oldFingerIndex << " higher - tmp: " << tmp << " (" << tmp->value_
			 << ") oldest: " << oldest_ << " (" << oldestValue << ")" << std::endl;
	    }
	}
	else if (oldestValue > newValue) {
	    shiftBigger = false;
	    size_t oldFingerIndex = findFingerIndexAtOrAfterOldest();
	    LOGDEBUG << "oldestValue > newValue - oldFingerIndex: " << oldFingerIndex << std::endl;

	    while (fingers_[oldFingerIndex]->value_ >= newValue) {
		Node* tmp = fingers_[oldFingerIndex];
		shifts.push_back(oldFingerIndex);
		LOGDEBUG << "will shift " << oldFingerIndex << " lower - tmp: " << tmp << " (" << tmp->value_
			 << ") oldest: " << oldest_ << " (" << oldestValue << ")" << std::endl;
		if (oldFingerIndex == 0)
		    break;
		--oldFingerIndex;
	    }
	}

	// Now resuse the oldest node and prepare to move it to its new location in the ordered list.
	//
	Node* tmp = storeValue(newValue);
	if (left == tmp) left = left->smaller_;
	if (right == tmp) right = right->bigger_;

	Node* smaller = tmp->smaller_;
	Node* bigger = tmp->bigger_;

	// Unlink the Node from the ordered chain. WARNING: the operations from here to the end of this scope
	// are very order dependent; change at your peril.
	//
	if (! smaller) {
	    bigger->smaller_ = 0;
	}
	else if (! bigger) {
	    smaller->bigger_ = 0;
	}
	else {
	    smaller->bigger_ = bigger;
	    bigger->smaller_ = smaller;
	}

	LOGDEBUG << "tmp: " << tmp << " left: " << left << " right: " << right << " min: " << minValue_
                 << " max: " << maxValue_ << std::endl;

	// Begin to relink the Node into the ordered chain. However, DO NOT update the new Node's ordered links
	// until after we apply any shifts to the finger Nodes, since the finger Nodes could point to the old
	// node.
	//
	if (left) left->bigger_ = tmp;
	if (right) right->smaller_ = tmp;
	
	// Now, apply the shifts.
	//
	if (shiftBigger) {
	    std::for_each(shifts.begin(), shifts.end(), boost::bind(&RunningMedian::shiftFingerBigger,
                                                                    this, _1));
        }
	else {
	    std::for_each(shifts.begin(), shifts.end(), boost::bind(&RunningMedian::shiftFingerSmaller,
                                                                    this, _1));
        }

	// Finally, complete the relinking of the old node into its proper place in the ordered linked-list.
	//
	tmp->bigger_ = right;
	if (! right) maxValue_ = newValue;

	tmp->smaller_ = left;
	if (! left) minValue_ = newValue;
    }

    LOGDEBUG3 << "AFTER" << std::endl;
    if (log.showsDebug3()) dump();

    // Update the median value. Locate the proper node.
    //
    Node* tmp = fingers_[medianFingerIndex_];
    for (size_t index = 0; index < medianFingerOffset_; ++index) {
	tmp = tmp->bigger_;
    }

    if (isOdd_) {
	median_ = tmp->value_;
    }
    else {
	median_ = (tmp->value_ + tmp->bigger_->value_) / 2;
    }

    LOGDEBUG << "median: " << median_ << std::endl;
    
    return median_;
}

void
RunningMedian::dump() const
{
    Logger::ProcLog log("dump", Log());
    LOGINFO << std::endl;

    Node* tmp = oldest_;
    LOGDEBUG << "Oldest -> Youngest:" << std::endl;
    while (tmp) {
	LOGDEBUG << tmp << " value: " << tmp->value_ << " smaller: " << tmp->smaller_ << " bigger: "
                 << tmp->bigger_ << std::endl;
	tmp = tmp->younger_;
    }

    LOGDEBUG << "Smallest -> Biggest" << std::endl;
    tmp = fingers_[0];
    while (tmp) {
	LOGDEBUG << tmp << " value: " << tmp->value_ << std::endl;
	tmp = tmp->bigger_;
    }

    LOGDEBUG << "Fingers:" << std::endl;
    for (size_t index = 0; index < fingers_.size(); ++index) {
	Node* goal = 0;
	if (index < fingers_.size() - 1) {
	    goal = fingers_[index + 1];
        }
	int span = 0;
	tmp = fingers_[index];
	while (tmp != goal) {
	    ++span;
	    tmp = tmp->bigger_;
	}

	if (fingers_[index]) {
	    LOGDEBUG << fingers_[index] << " value: " << fingers_[index]->value_ << " span: " << span
		     << " next: " << goal << std::endl;
	}
	else {
	    LOGDEBUG << fingers_[index] << " NULL!!!" << std::endl;
	}
    }

    LOGDEBUG << "Min: " << minValue_ << " Max: " << maxValue_ << std::endl;
}

void
RunningMedian::clear(double initialValue)
{
    // Easy peezy: just replace all of the values with the initial one. The result is of course sorted, and we
    // don't have to move any nodes around.
    //
    for (Node* node = oldest_; node; node = node->younger_) {
	node->value_ = initialValue;
    }
    median_ = initialValue;
    sum_ = initialValue * windowSize_;
    minValue_ = maxValue_ = initialValue;
}

size_t
RunningMedian::findLargestFingerSmallerThan(double value) const
{
    size_t index = fingers_.size();
    while (index > 0) {
	if (fingers_[--index]->value_ < value)
	    break;
    }
    return index;
}

size_t
RunningMedian::findFingerIndexAtOrBeforeOldest() const
{
    // Locate the first finger whose value is less than or equal to the oldest value in the set. If we found the
    // oldest value, just return the index of the finger.
    //
    double oldestValue = oldest_->value_;
    size_t index = fingers_.size();
    while (index > 0) {
	--index;
	if (fingers_[index]->value_ < oldestValue)
	    break;
	if (fingers_[index] == oldest_)
	    return index;
    }

    // We need to walk forward to until we locate the oldest node, incrementing our index value each time we
    // come to a finger node.
    //
    Node* node = fingers_[index++]->bigger_;
    while (node != oldest_ && index < fingers_.size() - 1) {
	node = node->bigger_;
	if (node == fingers_[index])
	    ++index;
    }

    return index;
}

size_t
RunningMedian::findFingerIndexAtOrAfterOldest() const
{
    // Locate the first finger whose value is greater than or equal to the oldest value in the set. If we found
    // the oldest value, just return the index of the finger.
    //
    double oldestValue = oldest_->value_;
    size_t index = 0;
    for (; index < fingers_.size(); ++index) {
	if (fingers_[index]->value_ > oldestValue)
	    break;
	if (fingers_[index] == oldest_)
	    return index;
    }

    // Always return a valid index
    //
    if (index == fingers_.size())
	return index - 1;

    Node* node = fingers_[index--]->smaller_;
    while (node != oldest_) {
	if (node == fingers_[index])
	    --index;
	node = node->smaller_;
    }

    return index;
}

double
RunningMedian::getTrueMeanValue()
{
    sum_ = 0.0;
    for (Node* node = oldest_; node; node = node->younger_)
	sum_ += node->value_;
    return getEstimatedMeanValue();
}

RunningMedian::OrderedIndex
RunningMedian::generateOrderedIndex(size_t index) const
{
    Logger::ProcLog log("generateOrderedIndex", Log());
    LOGINFO << "index: " << index << " windowSize: " << windowSize_ << " fingerSpan: " << fingerSpan_
            << std::endl;

    if (index >= windowSize_)
	throw Utils::Exception("index must be less than window size");

    size_t fingerIndex = index / fingerSpan_;
    LOGDEBUG << "fingerIndex: " << fingerIndex << std::endl;
    size_t offset = index - fingerIndex * fingerSpan_;
    LOGDEBUG << "offset: " << offset << std::endl;
    return OrderedIndex(fingerIndex, offset);
}

double
RunningMedian::getOrderedValue(const OrderedIndex& index) const
{
    Logger::ProcLog log("getOrderedValue", Log());
    LOGINFO << "index: " << index.index_ << "/" << index.offset_ << std::endl;
    Node* node = fingers_[index.index_];
    LOGDEBUG << "node: " << node << std::endl;
    for (size_t offset = 0; offset < index.offset_; ++offset) {
	node = node->bigger_;
	LOGDEBUG << "node: " << node << std::endl;
    }
    LOGDEBUG << "value: " << node->value_ << std::endl;
    return node->value_;
}
