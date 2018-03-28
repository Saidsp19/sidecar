#ifndef SIDECAR_MESSAGES_DECIMATOR_H // -*- C++ -*-
#define SIDECAR_MESSAGES_DECIMATOR_H

#include "boost/iterator/iterator_facade.hpp"
#include <stdexcept>

#include "IO/Printable.h"
#include "Messages/PRIMessage.h"

namespace SideCar {
namespace Messages {

/** Template class that can generate iterators for a TPRIMessage class that return every N item. The template
    parameter _M should be a class derived or instantiate from TPRIMessage.
*/
template <typename _M>
class TDecimator : public Header, public IO::Printable<TDecimator<_M>> {
public:
    using Self = TDecimator<_M>;
    using Ref = boost::shared_ptr<Self>;
    using DatumType = typename _M::DatumType;
    using SourceConstIterator = typename _M::const_iterator;
    using SourceRef = typename _M::Ref;

    /** Create a new decimator

        \param source

        \param factor

        \return
    */
    static Ref Make(const SourceRef& source, int factor)
    {
        Ref ref(new TDecimator<_M>(source, factor));
        return ref;
    }

    size_t getMessageSize() const { return source_->Header::getMessageSize() + size() * sizeof(DatumType); }

    /** Obtain the number of elements in the decimated sequence

        \return size
    */
    size_t size() const { return (source_->size() + factor_ - 1) / factor_; }

    /** Determine if the decimated sequence is empty.

        \return
    */
    bool empty() const { return source_->empty(); }

    /** Internal class that defines a constant iterator for the decimated sequence. Relies on
        boost::iterator_facade to do the hard stuff.
    */
    class const_iterator
        : public boost::iterator_facade<const_iterator, DatumType const, boost::random_access_traversal_tag> {
    public:
        using Base = boost::iterator_facade<const_iterator, DatumType const, boost::random_access_traversal_tag>;

        const_iterator() : cit_(), factor_() {}

        const_iterator(SourceConstIterator cit, int factor) : cit_(cit), factor_(factor) {}

    private:
        friend class boost::iterator_core_access;

        void increment() { cit_ += factor_; }
        void decrement() { cit_ -= factor_; }
        void advance(ptrdiff_t n) { cit_ += n * factor_; }

        ptrdiff_t distance_to(const_iterator const& other) const { return std::distance(cit_, other.cit_) / factor_; }

        bool equal(const_iterator const& other) const { return cit_ == other.cit_; }

        DatumType const& dereference() const { return *cit_; }

        SourceConstIterator cit_;
        int factor_;
    };

    /** Obtain a read-only iterator to the first element in the decimated sequence.

        \return read-only iterator
    */
    const_iterator begin() const { return const_iterator(source_->begin(), factor_); }

    /** Obtain a read-only iterator to the termination point of the decimated sequence. NOTE: I think this
        violates the C++ standard...

        \return
    */
    const_iterator end() const { return const_iterator(source_->begin() + size() * factor_, factor_); }

    /** Implementation of the Base abstact method. Forwards the load to the underlying container. NOTE: this
        should normally NOT be used.

        \param cdr

        \return
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr) { return source_->load(cdr); }

    /** Implementation of the Base abstact method. Write out the decimated sequence to a CDR output stream.

        \param cdr stream to write to

        \return stream written to
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const
    {
        source_->PRIMessageBase::write(cdr, size());
        const_iterator p = begin();
        const_iterator e = end();
        while (p != e) cdr << *p++;
        return cdr;
    }

    std::ostream& print(std::ostream& os) const
    {
        os << "DecimationFactor: " << factor_ << '\n';
        return source_->print(os);
    }

private:
    /** Constructor.

        \param source container holding the data to decimate

        \param factor striping factor.
    */
    TDecimator(const SourceRef& source, size_t factor) : Header(*source), source_(source), factor_(factor)
    {
        if (factor < 1) throw std::out_of_range("factor value");
    }

    SourceRef source_;
    size_t factor_;
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
