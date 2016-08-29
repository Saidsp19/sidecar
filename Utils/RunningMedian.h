#ifndef UTILS_RUNNINGMEDIAN_H // -*- C++ -*-
#define UTILS_RUNNINGMEDIAN_H

#include <vector>

namespace Logger { class Log; }

namespace Utils {

/** Container class the maintains a running median (and mean) value for a stream of values. Instances have a
    fixed window size that determines how many samples to track for median calculations; after receiving this
    number of samples, the algorithm starts to forget the oldest sample.

    The basics of the algorithm were taken from the paper "Efficient Algorithm for computing a Running Median"
    by Soumya D. Mohanty for the CalTech/MIT LIGO project. The key insight from the paper is the management of
    an array of equidistant 'checks' or 'fingers' that point to nodes of an ordered link-list. These nodes offer
    improved search speed over walking the linked list, with the increased complexity necessary to manage them
    as the elements in the linked-list move around.

    Although the algorithm described is sound, the C implementation in the paper has some bugs that affected
    performance. In my implementation, I use the term 'fingers' to refer to the entity the paper calls a
    'check'.

    TODO: since the fingers are always ordered, an even faster implementation would be to use binary search on
    them to find the one to use to locate a given ordered link-list node.
*/
class RunningMedian 
{
public:

    /** Representation of an address to a specific ordered value in the RunningMedian container. Use the
	RunningMedian::generateOrderedIndex() method to create instances of this class.
    */
    class OrderedIndex {
	OrderedIndex(size_t index, size_t offset) : index_(index), offset_(offset) {}
	size_t index_;
	size_t offset_;
	friend class RunningMedian;
    };

    /** Obtain Logger::Log device to use for instances of this class.

        \return Logger::Log reference
    */
    static Logger::Log& Log();

    /** Constructor. Creates a new container for windowSize samples, initialized to initialValue values.

        \param windowSize number of samples to remember

        \param initialValue initial value to give to window samples
    */
    RunningMedian(size_t windowSize, double initialValue = 0.0);

    /** Destructor.
     */
    ~RunningMedian();

    /** Add a new sample to the container, and obtain an updated median value.

        \param value sample value to add

        \return updated median value
    */
    double addValue(double value);

    /** Obtain the current median value for the samples seen so far.

        \return current median value
    */
    double getMedianValue() const { return median_; }

    /** Obtain an estimated mean (average) value of the samples seen so far. For speed, the RunningMedian class
        maintains a running sum of the sample values, and this method simply divides the running sum by the
        window size to obtain a mean value. However, note that after some amount of time, this calculation will
        most likely deviate from the true mean value due to rounding errors, especially when the magnitude
        between the largest and smallest values is very large. One can always obtain a true mean value via
        getTrueMeanValue() method, at some cost in performance.

	\return current (estimated) mean value
    */
    double getEstimatedMeanValue() const { return sum_ / windowSize_; }

    /** Obtain the true mean (average) value of the samples seen so far. Unlike, getMeanValue(), this method
        calculates the sum of the samples from scratch.

        \return true mean value
    */
    double getTrueMeanValue();

    /** Obtain the minimum value of all of the samples held by the container

        \return running minimum value
    */
    double getMinimumValue() const { return minValue_; }

    /** Obtain the maximum value of all of the samples held by the container

        \return running maximum value
    */
    double getMaximumValue() const { return maxValue_; }

    /** Obtain an OrderedIndex object that corresponds to a given offset in the sample window. For instance, an
        index value of zero (0) would return an OrderedIndex object that refers to the smallest element in the
        container, and a value of windowSize_ - 1 would return an OrderedIndex for the largest value.

        \param index ordered sample in the running window to refer to. Value must be < windowSize_.

        \return OrderedIndex object
    */
    OrderedIndex generateOrderedIndex(size_t index) const;

    /** Obtain the value for the sample in the ordered window at the given location.

        \param index location to fetch

        \return current value at the given location
    */
    double getOrderedValue(const OrderedIndex& index) const;

    /** Convenience method for fetching ordered sample values without dealing with OrderedIndex objects.

        \param index offset from the start of the running window to fetch

        \return current value at the given location
    */
    double getOrderedValue(size_t index) const { return getOrderedValue(generateOrderedIndex(index)); }

    /** Reset the container so that all samples have a given initial value.

        \param initialValue the initial value to use
    */
    void clear(double initialValue = 0.0);

    /** Reset the container so that it has a new sample window size. If the given window size is the same as the
        current one, the effect will be the same as calling clear(initialValue).

        \param windowSize new size to use

        \param initialValue the initial value to use for samples
    */
    void setWindowSize(size_t windowSize, double initialValue = 0.0);

    /** Write out a textual representation of the internal container to the RunningMedian log device. Only
	useful for debugging purposes.
    */
    void dump() const;

private:

    /** Internal Node object used to store sample values. Provides links so that the sample can exist in a
	temporal FIFO linked-list as well as a numerically-ordered doubly-linked list.
    */
    struct Node {
	double value_;
	Node* younger_;
	Node* smaller_;
	Node* bigger_;
    };

    /** Release allocated Node objects.
     */
    void releaseNodes();

    /** Have a given finger refer to the next Node in the ordered link-list.

        \param index the finger to shift
    */
    void shiftFingerBigger(size_t index) { fingers_[index] = fingers_[index]->bigger_; }

    /** Have a given finger refer to the previous Node in the ordered link-list.

        \param index the finger to shift
    */
    void shiftFingerSmaller(size_t index) { fingers_[index] = fingers_[index]->smaller_; }

    /** Locate the finger with the largest value that is smaller than a given value.

        \param value the value to compare

        \return finger index
    */
    size_t findLargestFingerSmallerThan(double value) const;

    size_t findFingerIndexAtOrBeforeOldest() const;

    size_t findFingerIndexAtOrAfterOldest() const;

    /** Store a value in the oldest Node, relinking the Node in the temporal linked-list to make it the
        youngest.

        \param value the sample value to store in the Node

        \return the youngest Node
    */
    Node* storeValue(double value);

    Node* oldest_;
    Node* youngest_;
    std::vector<Node*> fingers_;
    size_t medianFingerIndex_;
    size_t medianFingerOffset_;
    size_t windowSize_;
    size_t fingerSpan_;
    double median_;
    double sum_;
    double minValue_;
    double maxValue_;
    bool isOdd_;
};

} // end namespace Utils

#endif
